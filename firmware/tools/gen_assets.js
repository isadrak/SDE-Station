// Génère src/web/assets.h à partir du dossier app/ (fichiers compressés en gzip).
// Usage (depuis firmware/) : node tools/gen_assets.js
const fs = require('fs'), path = require('path'), zlib = require('zlib');
const APP = path.join(__dirname, '..', '..', 'app');
const OUT = path.join(__dirname, '..', 'src', 'web', 'assets.h');
const FILES = [
  ['/', 'sde-analyse.html', 'text/html; charset=utf-8'],
  ['/manifest.webmanifest', 'manifest.webmanifest', 'application/manifest+json'],
  ['/sw.js', 'sw.js', 'text/javascript'],
  ['/icon-192.png', 'icon-192.png', 'image/png'],
  ['/icon-512.png', 'icon-512.png', 'image/png'],
  ['/icon-maskable.png', 'icon-maskable.png', 'image/png'],
  ['/apple-touch-icon.png', 'apple-touch-icon.png', 'image/png'],
];
let arrays = '', table = '', total = 0;
FILES.forEach(([url, file, mime], i) => {
  let raw = fs.readFileSync(path.join(APP, file));
  // Page : sans l'indentation ni les lignes de commentaire (« // ... »), pour
  // gagner ~7 ko de flash. Le fichier source reste lisible.
  if (file.endsWith('.html')) raw = Buffer.from(raw.toString('utf8').split('\n').map(l => l.trim())
    .filter(l => l && !l.startsWith('//')).join('\n'));
  const gz = !mime.startsWith('image/');            // les PNG sont déjà compressés
  const data = gz ? zlib.gzipSync(raw, { level: 9 }) : raw;
  total += data.length;
  let body = '';
  for (let k = 0; k < data.length; k += 24)
    body += '  ' + Array.from(data.subarray(k, k + 24)).map(b => '0x' + b.toString(16).padStart(2, '0')).join(',') + ',\n';
  arrays += `// ${file} (${raw.length} o${gz ? ', gzip ' + data.length + ' o' : ''})\nstatic const uint8_t ASSET_${i}[] PROGMEM = {\n${body}};\n\n`;
  table += `  {"${url}", "${mime}", ASSET_${i}, ${data.length}, ${gz}},\n`;
  console.log(url.padEnd(24), data.length);
});
fs.writeFileSync(OUT, `#pragma once
// Généré automatiquement par tools/gen_assets.js depuis le dossier app/ : ne pas éditer.
#include <Arduino.h>

struct WebAsset {
  const char* path;
  const char* mime;
  const uint8_t* data;
  size_t len;
  bool gz;
};

${arrays}static const WebAsset WEB_ASSETS[] = {
${table}};
`);
console.log('total', total);
