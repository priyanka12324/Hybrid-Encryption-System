/*
 * HES-CQCT cipher, ported from CryptoEngine.cpp.
 * Works on BYTES (like std::string in C++), so UTF-8 text round-trips.
 *
 *   K1: byte ^= K1[i % |K1|] ^ (nonce % 256) ^ (i % 256)
 *   K2: byte += (K2[i % |K2|] + nonce) % 256          (mod 256)
 *   K3: rotate whole message by r = (sum(K3) + nonce % 256) % len
 *       even r -> rotate right, odd r -> rotate left; r = 0 -> no rotation
 *
 * Keys must be printable ASCII. The C++ code uses signed `char`, so
 * non-ASCII keys would behave differently there; we reject them.
 *
 * This is an educational cipher, not a vetted one. Do not protect real data with it.
 */
(function (root) {
  "use strict";

  const KEY_RE = /^[\x20-\x7E]{1,64}$/;

  function keyBytes(s) {
    const out = new Uint8Array(s.length);
    for (let i = 0; i < s.length; i++) out[i] = s.charCodeAt(i);
    return out;
  }

  function validateKeys(keys) {
    for (const name of ["k1", "k2", "k3"]) {
      if (typeof keys[name] !== "string" || !KEY_RE.test(keys[name])) {
        return name.toUpperCase() + " must be 1 to 64 printable ASCII characters.";
      }
    }
    return null;
  }

  // r and direction depend only on K3, nonce and message length.
  function rotation(len, nonce, k3) {
    if (len === 0) return { r: 0, dir: "none" };
    let sum = 0;
    for (const b of keyBytes(k3)) sum += b;
    const r = (sum + (nonce % 256)) % len;
    if (r === 0) return { r: 0, dir: "none" };
    return { r, dir: r % 2 === 0 ? "right" : "left" };
  }

  function rotateLeft(a, r) {
    return Uint8Array.from([...a.subarray(r), ...a.subarray(0, r)]);
  }
  function rotateRight(a, r) {
    return rotateLeft(a, a.length - r);
  }

  function encrypt(plain, nonce, keys) {
    const K1 = keyBytes(keys.k1), K2 = keyBytes(keys.k2);
    const k1 = new Uint8Array(plain.length);
    const k2 = new Uint8Array(plain.length);
    for (let i = 0; i < plain.length; i++) {
      k1[i] = (plain[i] ^ K1[i % K1.length] ^ (nonce % 256) ^ (i % 256)) & 0xff;
      const shift = (K2[i % K2.length] + nonce) % 256;
      k2[i] = (k1[i] + shift) & 0xff;
    }
    const rot = rotation(plain.length, nonce, keys.k3);
    let out = k2;
    if (rot.r > 0) out = rot.dir === "right" ? rotateRight(k2, rot.r) : rotateLeft(k2, rot.r);
    return { k1, k2, out, r: rot.r, dir: rot.dir };
  }

  function decrypt(cipher, nonce, keys) {
    const K1 = keyBytes(keys.k1), K2 = keyBytes(keys.k2);
    const rot = rotation(cipher.length, nonce, keys.k3);
    let k2 = cipher;                                   // after undoing K3
    if (rot.r > 0) k2 = rot.dir === "right" ? rotateLeft(cipher, rot.r) : rotateRight(cipher, rot.r);
    const k1 = new Uint8Array(cipher.length);          // after undoing K2
    const plain = new Uint8Array(cipher.length);       // after undoing K1
    for (let i = 0; i < cipher.length; i++) {
      const shift = (K2[i % K2.length] + nonce) % 256;
      k1[i] = (k2[i] - shift) & 0xff;
      plain[i] = (k1[i] ^ K1[i % K1.length] ^ (nonce % 256) ^ (i % 256)) & 0xff;
    }
    return { k2, k1, plain, r: rot.r, dir: rot.dir };
  }

  function toHex(a) {
    return Array.from(a, (b) => b.toString(16).padStart(2, "0")).join(" ").toUpperCase();
  }
  function fromHex(h) {
    const s = h.replace(/\s+/g, "");
    const out = new Uint8Array(s.length / 2);
    for (let i = 0; i < out.length; i++) out[i] = parseInt(s.substr(i * 2, 2), 16);
    return out;
  }

  // 6-digit nonce in [100000, 999999], like generateNonce(), but drawn from the
  // browser's CSPRNG instead of a clock-seeded mt19937_64 (see README).
  function newNonce() {
    const range = 900000;
    const limit = Math.floor(0x100000000 / range) * range;
    const buf = new Uint32Array(1);
    do { root.crypto.getRandomValues(buf); } while (buf[0] >= limit);
    return 100000 + (buf[0] % range);
  }

  const api = { validateKeys, rotation, encrypt, decrypt, toHex, fromHex, newNonce };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  root.HES = api;
})(typeof window !== "undefined" ? window : globalThis);

