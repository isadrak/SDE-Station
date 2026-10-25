// Service worker SDE : l'appli s'ouvre même si la station est injoignable.
// La page est demandée au réseau d'abord (4 s au plus), puis au cache ; les
// icônes et le manifeste viennent du cache et sont rafraîchis en arrière-plan.
// Les données (/api) ne sont jamais mises en cache.
// Chemins relatifs au dossier du service worker : la même appli sert sur la
// station (http://sde.local/) et en ligne (https://….github.io/sde/).
const CACHE = 'sde-v13';
const ROOT = self.registration.scope;   // finit par « / »
const SHELL = ['', 'manifest.webmanifest', 'icon-192.png', 'icon-512.png'].map(p => new URL(p, ROOT).href);

self.addEventListener('install', e => {
  e.waitUntil(caches.open(CACHE).then(c => c.addAll(SHELL)).then(() => self.skipWaiting()));
});

self.addEventListener('activate', e => {
  e.waitUntil(caches.keys()
    .then(keys => Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k))))
    .then(() => self.clients.claim()));
});

self.addEventListener('fetch', e => {
  const url = new URL(e.request.url);
  if (e.request.method !== 'GET' || url.origin !== location.origin || url.pathname.includes('/api/')) return;
  const nav = e.request.mode === 'navigate';
  const key = nav ? ROOT : url.origin + url.pathname;   // ?app=1 et la page seule sont la même page
  // La page elle-même : le réseau d'abord (4 s au plus), pour qu'une mise à
  // jour du firmware montre tout de suite la nouvelle appli. Le cache ne sert
  // que si la station ne répond pas.
  if (nav) {
    e.respondWith(caches.open(CACHE).then(async c => {
      const ctl = new AbortController(), tm = setTimeout(() => ctl.abort(), 4000);
      try {
        const res = await fetch(e.request, { signal: ctl.signal, cache: 'no-store' });
        clearTimeout(tm);
        if (res.ok && res.type === 'basic') { e.waitUntil(c.put(key, res.clone())); return res; }
        return (await c.match(key)) || res;
      } catch {
        clearTimeout(tm);
        return (await c.match(key)) || Response.error();
      }
    }));
    return;
  }
  e.respondWith(caches.open(CACHE).then(async c => {
    const cached = await c.match(key);
    // Avec une copie en cache, la vérification attend 10 s : la station sert
    // une requête à la fois, et la première lecture des mesures passe avant.
    const wait = cached ? new Promise(r => setTimeout(r, 10000)) : Promise.resolve();
    const fresh = wait.then(() => fetch(e.request)).then(async res => {
      if (!res.ok || res.type !== 'basic') return res;
      if (nav && cached) {
        const [a, b] = await Promise.all([cached.clone().text(), res.clone().text()]);
        if (a !== b) {
          const clients = await self.clients.matchAll({ type: 'window' });
          for (const cl of clients) cl.postMessage('sde-update');
        }
      }
      await c.put(key, res.clone());
      return res;
    }).catch(() => null);
    if (cached) { e.waitUntil(fresh); return cached; }
    return (await fresh) || (await c.match(ROOT)) || Response.error();
  }));
});
