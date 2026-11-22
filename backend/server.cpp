// HES-CQCT chat relay in C++ (replaces the old Python server).
//
// The server is deliberately "dumb": it never sees keys or plaintext. It receives
// (ciphertext, nonce) pairs, stores them in YOUR circular queue (queue.cpp, unchanged),
// writes YOUR audit log (logger.cpp, unchanged) and streams the queue to every browser,
// which decrypts locally with cipher.js.
//
// Single-threaded: one select() loop serves every browser. No std::thread / std::mutex, so it
// also builds with old MinGW (GCC 6.3) and needs no locking around queue.cpp / logger.cpp.
//
// Build:  g++ -std=c++17 -O2 backend/cpp/server.cpp -o server.exe -lws2_32      (Windows)
//         g++ -std=c++17 -O2 backend/cpp/server.cpp -o server                    (Linux / macOS)
// Run:    server [--port 5000] [--capacity 5] [--frontend <dir>]
// audit_log.txt is created in the directory you run it from (that is how logger.cpp works).

#ifdef _WIN32
  #define NOMINMAX
  #define WIN32_LEAN_AND_MEAN
  #define FD_SETSIZE 512
  #include <winsock2.h>
  typedef SOCKET sock_t;
  typedef int socklen_type;
  #define CLOSESOCK closesocket
  #define BAD_SOCK INVALID_SOCKET
#else
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <sys/time.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <cerrno>
  #include <csignal>
  typedef int sock_t;
  typedef socklen_t socklen_type;
  #define CLOSESOCK close
  #define BAD_SOCK (-1)
#endif
#ifndef MSG_NOSIGNAL
  #define MSG_NOSIGNAL 0
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

#include "queue.cpp"      // struct Node, struct cq, createQueue, enqueue, dequeue, ... (+ logger.cpp)

using namespace std;

// ------------------------------------------------------------------ state ----
// queue.cpp's Node only holds (cipher bytes, nonce). The chat also needs id/name/cid/time, so a
// parallel deque of Meta mirrors the queue 1:1 (FIFO order, popped exactly when the queue evicts).
struct Meta { long long id; string name, cid; long long nonce; string cipherHex; double ts; };

struct Client {                      // one connected browser (a normal request or a live /stream)
    sock_t s;
    string in, out;                  // bytes received / bytes waiting to be sent
    bool stream, closeAfter, dead;   // stream = long-lived SSE connection
    string name, cid;                // only for stream clients
    double born, lastSend;
};

static struct cq* mq = nullptr;
static deque<Meta> metas;
static vector<Client*> clients;
static long long nextId = 1;
static string frontendDir;

// ---------------------------------------------------------------- helpers ----
static double nowSec() {
    return chrono::duration<double>(chrono::system_clock::now().time_since_epoch()).count();
}
static string fmtTs(double t) { ostringstream o; o << fixed << setprecision(3) << t; return o.str(); }

static string jesc(const string& s) {             // JSON string escape (UTF-8 passes through)
    string o;
    for (unsigned char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
        else o += (char)c;
    }
    return o;
}

static bool isHex(const string& s) {
    if (s.empty() || s.size() % 2 || s.size() > 8000) return false;
    for (unsigned char c : s) if (!isxdigit(c)) return false;
    return true;
}
static string hexDecode(const string& h) {
    string o;
    for (size_t i = 0; i + 1 < h.size(); i += 2) o += (char)stoi(h.substr(i, 2), nullptr, 16);
    return o;
}
static string lower(string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }

static bool validCid(const string& c) {
    if (c.size() < 8 || c.size() > 64) return false;
    for (unsigned char ch : c) if (!isalnum(ch)) return false;
    return true;
}

static string cleanName(const string& raw, const string& fallback = "") {   // trim + max 30 characters
    size_t a = raw.find_first_not_of(" \t\r\n"), b = raw.find_last_not_of(" \t\r\n");
    if (a == string::npos) return fallback;
    string t = raw.substr(a, b - a + 1), o;
    int chars = 0;
    for (unsigned char c : t) {
        if ((c & 0xC0) != 0x80) { if (chars == 30) break; chars++; }       // count code points
        o += (char)c;
    }
    return o;
}

static void putUtf8(string& o, unsigned cp) {
    if (cp < 0x80) o += (char)cp;
    else if (cp < 0x800) { o += (char)(0xC0 | cp >> 6); o += (char)(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { o += (char)(0xE0 | cp >> 12); o += (char)(0x80 | (cp >> 6 & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
    else { o += (char)(0xF0 | cp >> 18); o += (char)(0x80 | (cp >> 12 & 0x3F)); o += (char)(0x80 | (cp >> 6 & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
}

static bool parseJsonStr(const string& s, size_t& i, string& out) {
    if (i >= s.size() || s[i] != '"') return false;
    i++;
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') return true;
        if (c != '\\') { out += c; continue; }
        if (i >= s.size()) return false;
        char e = s[i++];
        switch (e) {
            case 'n': out += '\n'; break;  case 't': out += '\t'; break;  case 'r': out += '\r'; break;
            case 'b': out += '\b'; break;  case 'f': out += '\f'; break;
            case '"': case '\\': case '/': out += e; break;
            case 'u': {
                auto hex4 = [&](unsigned& v) {
                    if (i + 4 > s.size()) return false;
                    for (int k = 0; k < 4; k++) if (!isxdigit((unsigned char)s[i + k])) return false;
                    v = (unsigned)stoul(s.substr(i, 4), nullptr, 16); i += 4; return true; };
                unsigned cp;
                if (!hex4(cp)) return false;
                if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                    i += 2; unsigned lo;
                    if (!hex4(lo)) return false;
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                }
                putUtf8(out, cp); break;
            }
            default: return false;
        }
    }
    return false;
}

// Minimal parser for a flat JSON object of string/number values, e.g. {"name":"A","nonce":123456,...}
static bool parseFlatJson(const string& s, map<string, string>& out) {
    size_t i = 0;
    auto ws = [&] { while (i < s.size() && isspace((unsigned char)s[i])) i++; };
    ws();
    if (i >= s.size() || s[i] != '{') return false;
    i++;
    for (;;) {
        ws();
        if (i < s.size() && s[i] == '}') return true;
        string key, val;
        if (!parseJsonStr(s, i, key)) return false;
        ws();
        if (i >= s.size() || s[i] != ':') return false;
        i++; ws();
        if (i < s.size() && s[i] == '"') { if (!parseJsonStr(s, i, val)) return false; }
        else { size_t st = i; while (i < s.size() && s[i] != ',' && s[i] != '}' && !isspace((unsigned char)s[i])) i++; val = s.substr(st, i - st); }
        out[key] = val;
        ws();
        if (i < s.size() && s[i] == ',') i++;
    }
}

static string urlDecode(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size() + 0 && isxdigit((unsigned char)s[i + 1]) && isxdigit((unsigned char)s[i + 2])) {
            o += (char)stoi(s.substr(i + 1, 2), nullptr, 16); i += 2;
        } else if (s[i] == '+') o += ' ';
        else o += s[i];
    }
    return o;
}
static map<string, string> parseQuery(const string& q) {
    map<string, string> m; size_t p = 0;
    while (p <= q.size()) {
        size_t amp = q.find('&', p); if (amp == string::npos) amp = q.size();
        string kv = q.substr(p, amp - p); size_t eq = kv.find('=');
        if (eq != string::npos) m[urlDecode(kv.substr(0, eq))] = urlDecode(kv.substr(eq + 1));
        p = amp + 1;
    }
    return m;
}

// ----------------------------------------------------------------- events ----
static string msgJson(const Meta& m) {
    return "{\"kind\":\"msg\",\"id\":" + to_string(m.id) + ",\"name\":\"" + jesc(m.name) + "\",\"cid\":\"" + m.cid +
           "\",\"nonce\":" + to_string(m.nonce) + ",\"cipher\":\"" + m.cipherHex + "\",\"ts\":" + fmtTs(m.ts) + "}";
}
static string snapshotJson() {
    string o = "{\"kind\":\"queue\",\"capacity\":" + to_string(mq->max_capacity) + ",\"nodes\":[";
    for (size_t i = 0; i < metas.size(); i++) {
        const Meta& m = metas[i];
        if (i) o += ",";
        o += "{\"id\":" + to_string(m.id) + ",\"name\":\"" + jesc(m.name) + "\",\"cid\":\"" + m.cid +
             "\",\"nonce\":" + to_string(m.nonce) + ",\"cipher\":\"" + m.cipherHex + "\"}";
    }
    return o + "]}";
}
static string presenceJson() {                     // unique by cid, latest name wins
    vector<pair<string, string> > seen;
    for (size_t k = 0; k < clients.size(); k++) {
        Client* c = clients[k];
        if (!c->stream || c->dead) continue;
        bool found = false;
        for (size_t i = 0; i < seen.size(); i++) if (seen[i].first == c->cid) { seen[i].second = c->name; found = true; }
        if (!found) seen.push_back(make_pair(c->cid, c->name));
    }
    string o = "{\"kind\":\"presence\",\"people\":[";
    for (size_t i = 0; i < seen.size(); i++) {
        if (i) o += ",";
        o += "{\"cid\":\"" + seen[i].first + "\",\"name\":\"" + jesc(seen[i].second) + "\"}";
    }
    return o + "]}";
}
static void sse(Client* c, const string& ev) { c->out += "data: " + ev + "\n\n"; }
static void broadcast(const string& ev) {
    for (size_t i = 0; i < clients.size(); i++) if (clients[i]->stream && !clients[i]->dead) sse(clients[i], ev);
}

// ------------------------------------------------------------------- http ----
static void reply(Client* c, int code, const string& body, const string& ctype = "application/json") {
    const char* text = code == 200 ? "OK" : code == 400 ? "Bad Request" : "Not Found";
    c->out += "HTTP/1.1 " + to_string(code) + " " + text + "\r\nContent-Type: " + ctype +
              "\r\nContent-Length: " + to_string(body.size()) + "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n" + body;
    c->closeAfter = true;
}

static void serveStatic(Client* c, const string& file, const string& ctype) {
    ifstream f((frontendDir + "/" + file).c_str(), ios::binary);
    if (!f) return reply(c, 404, "{}");
    stringstream ss; ss << f.rdbuf();
    reply(c, 200, ss.str(), ctype);
}

static void handleSend(Client* c, const string& body) {
    map<string, string> f;
    if (!parseFlatJson(body, f) || !f.count("name") || !f.count("cid") || !f.count("nonce") || !f.count("cipher"))
        return reply(c, 400, "{\"error\":\"bad request\"}");
    string name = cleanName(f["name"]), cid = f["cid"], cipher = f["cipher"];
    cipher.erase(remove(cipher.begin(), cipher.end(), ' '), cipher.end());
    char* end = nullptr; long nonce = strtol(f["nonce"].c_str(), &end, 10);   // nonce <= 999999 fits in long
    if (name.empty() || !validCid(cid) || end == f["nonce"].c_str() || *end || nonce < 100000 || nonce > 999999 || !isHex(cipher))
        return reply(c, 400, "{\"error\":\"invalid fields\"}");

    Meta m;
    m.id = nextId++; m.name = name; m.cid = cid; m.nonce = nonce; m.cipherHex = lower(cipher); m.ts = nowSec();
    if (mq->size >= mq->max_capacity && !metas.empty()) metas.pop_front();   // queue.cpp will evict its front
    enqueue(mq, hexDecode(m.cipherHex), nonce);                              // logs EVICTED / ENQUEUE
    metas.push_back(m);
    broadcast(msgJson(m));
    broadcast(snapshotJson());
    cout << "#" << m.id << " " << name << "  nonce=" << nonce << "  cipher=" << m.cipherHex.substr(0, 48)
         << (m.cipherHex.size() > 48 ? "..." : "") << endl;
    reply(c, 200, "{\"ok\":true}");
}

static void handleStream(Client* c, const string& query) {
    map<string, string> q = parseQuery(query);
    string cid = q.count("cid") ? q["cid"] : "";
    if (!validCid(cid)) return reply(c, 400, "{\"error\":\"bad cid\"}");
    c->name = cleanName(q.count("name") ? q["name"] : "", "Guest");
    c->cid = cid;
    long last = q.count("last") ? atol(q["last"].c_str()) : 0;
    bool rejoin = q.count("rejoin") && q["rejoin"] == "1";

    c->out += "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-store\r\nConnection: keep-alive\r\n\r\n";
    c->stream = true;
    string hist = "{\"kind\":\"history\",\"events\":[";
    bool first = true;
    for (size_t i = 0; i < metas.size(); i++) if (metas[i].id > last) { if (!first) hist += ","; hist += msgJson(metas[i]); first = false; }
    sse(c, hist + "]}");
    sse(c, snapshotJson());
    broadcast(presenceJson());
    if (!rejoin) broadcast("{\"kind\":\"join\",\"name\":\"" + jesc(c->name) + "\",\"cid\":\"" + cid + "\",\"ts\":" + fmtTs(nowSec()) + "}");
    cout << c->name << " connected" << endl;
}

// Called once the whole request (headers + body) is in c->in.
static void tryHandle(Client* c) {
    size_t hdrEnd = c->in.find("\r\n\r\n");
    if (hdrEnd == string::npos) { if (c->in.size() > 65536) reply(c, 400, "{}"); return; }
    string head = c->in.substr(0, hdrEnd);
    istringstream rl(head); string method, target, ver; rl >> method >> target >> ver;
    size_t contentLen = 0;
    istringstream hs(head); string line; getline(hs, line);
    while (getline(hs, line)) {
        string l = lower(line);
        if (l.compare(0, 15, "content-length:") == 0) { long v = atol(l.c_str() + 15); contentLen = v < 0 ? 0 : (size_t)v; }
    }
    if (contentLen > 20000) return reply(c, 400, "{}");
    if (c->in.size() < hdrEnd + 4 + contentLen) return;                    // wait for the rest of the body
    string body = c->in.substr(hdrEnd + 4, contentLen);
    size_t qm = target.find('?');
    string path = target.substr(0, qm), query = qm == string::npos ? "" : target.substr(qm + 1);

    if (method == "GET" && (path == "/" || path == "/index.html")) serveStatic(c, "index.html", "text/html; charset=utf-8");
    else if (method == "GET" && path == "/cipher.js") serveStatic(c, "cipher.js", "application/javascript; charset=utf-8");
    else if (method == "GET" && path == "/stream") handleStream(c, query);
    else if (method == "POST" && path == "/send") handleSend(c, body);
    else reply(c, 404, "{}");
    c->in.clear();
}

// ---------------------------------------------------------------- sockets ----
static bool wouldBlock() {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
#endif
}
static void setNonBlocking(sock_t s) {
#ifdef _WIN32
    u_long mode = 1; ioctlsocket(s, FIONBIO, &mode);
#else
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
}

static void onReadable(Client* c) {
    char buf[4096];
    for (;;) {
        int n = (int)recv(c->s, buf, sizeof buf, 0);
        if (n > 0) {
            if (!c->stream && !c->closeAfter) { c->in.append(buf, n); tryHandle(c); }   // stream clients send nothing useful
            continue;
        }
        if (n == 0 || !wouldBlock()) c->dead = true;                         // peer closed or error
        return;
    }
}
static void onWritable(Client* c) {
    while (!c->out.empty()) {
        int n = (int)send(c->s, c->out.data(), (int)c->out.size(), MSG_NOSIGNAL);
        if (n > 0) { c->out.erase(0, (size_t)n); c->lastSend = nowSec(); continue; }
        if (n < 0 && wouldBlock()) return;
        c->dead = true; return;
    }
    if (c->closeAfter) c->dead = true;                                       // response fully sent
}

static string lanIp() {
    string ip = "127.0.0.1";
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != BAD_SOCK) {
        sockaddr_in a; memset(&a, 0, sizeof a);
        a.sin_family = AF_INET; a.sin_port = htons(1); a.sin_addr.s_addr = inet_addr("10.255.255.255");
        if (connect(s, (sockaddr*)&a, sizeof a) == 0) {                      // UDP connect: nothing is actually sent
            sockaddr_in me; memset(&me, 0, sizeof me); socklen_type len = sizeof me;
            if (getsockname(s, (sockaddr*)&me, &len) == 0) ip = inet_ntoa(me.sin_addr);
        }
        CLOSESOCK(s);
    }
    return ip;
}

int main(int argc, char** argv) {
    int port = 5000, cap = 5;
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        if (a == "--port" && i + 1 < argc) port = atoi(argv[++i]);
        else if (a == "--capacity" && i + 1 < argc) cap = atoi(argv[++i]);
        else if (a == "--frontend" && i + 1 < argc) frontendDir = argv[++i];
    }
    cap = max(2, min(cap, 60));
    if (frontendDir.empty()) {                                      // works from project root, backend/ or backend/cpp/
        const char* cands[] = {"frontend", "../frontend", "../../frontend"};
        for (int i = 0; i < 3; i++) if (ifstream((string(cands[i]) + "/index.html").c_str())) { frontendDir = cands[i]; break; }
    }
    if (frontendDir.empty() || !ifstream((frontendDir + "/index.html").c_str())) {
        cerr << "Cannot find the frontend folder. Run from the project folder or pass --frontend <dir>.\n";
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    mq = createQueue(cap);                                          // queue.cpp

    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
    sockaddr_in addr; memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_ANY); addr.sin_port = htons((unsigned short)port);
    if (bind(srv, (sockaddr*)&addr, sizeof addr) != 0 || listen(srv, 64) != 0) { cerr << "Cannot listen on port " << port << " (in use?)\n"; return 1; }

    cout << "\n  HES-CQCT chat (C++ server) is running.\n\n"
         << "  This computer:             http://localhost:" << port << "\n"
         << "  Other device (same Wi-Fi): http://" << lanIp() << ":" << port << "\n"
         << "  Queue capacity: " << cap << " messages. The server only ever sees ciphertext.\n"
         << "  Frontend: " << frontendDir << "   Audit log: ./audit_log.txt\n\n  Ctrl+C to stop.\n" << endl;

    for (;;) {
        fd_set rs, ws; FD_ZERO(&rs); FD_ZERO(&ws);
        FD_SET(srv, &rs);
        sock_t maxfd = srv;
        for (size_t i = 0; i < clients.size(); i++) {
            FD_SET(clients[i]->s, &rs);
            if (!clients[i]->out.empty()) FD_SET(clients[i]->s, &ws);
            if (clients[i]->s > maxfd) maxfd = clients[i]->s;
        }
        timeval tv; tv.tv_sec = 1; tv.tv_usec = 0;
        if (select((int)maxfd + 1, &rs, &ws, nullptr, &tv) < 0) continue;

        if (FD_ISSET(srv, &rs) && clients.size() < FD_SETSIZE - 8) {
            sock_t s = accept(srv, nullptr, nullptr);
            if (s != BAD_SOCK) {
                setNonBlocking(s);
                Client* c = new Client();
                c->s = s; c->stream = c->closeAfter = c->dead = false; c->born = c->lastSend = nowSec();
                clients.push_back(c);
            }
        }
        for (size_t i = 0; i < clients.size(); i++) {                // note: handlers may append to other clients' out
            Client* c = clients[i];
            if (c->dead) continue;
            if (FD_ISSET(c->s, &rs)) onReadable(c);
            if (!c->dead && !c->out.empty()) onWritable(c);          // try to flush right away (also covers new events)
        }
        double t = nowSec();
        for (size_t i = 0; i < clients.size(); i++) {
            Client* c = clients[i];
            if (c->stream && !c->dead && t - c->lastSend > 15) { c->out += ": ping\n\n"; onWritable(c); }   // keep-alive + detects dead peers
            if (!c->stream && !c->dead && t - c->born > 10) c->dead = true;                              // idle half-open request
        }
        for (size_t i = 0; i < clients.size();) {                    // reap dead clients
            Client* c = clients[i];
            if (!c->dead) { i++; continue; }
            clients.erase(clients.begin() + i);
            CLOSESOCK(c->s);
            if (c->stream) { cout << c->name << " disconnected" << endl; broadcast(presenceJson()); }
            delete c;
        }
    }
}

