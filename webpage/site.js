// Shared helpers for index.html and player.html
const TE = {
  loadData: () => fetch('emulators.json').then(r => r.json()),
  esc: s => String(s ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c])),
  launchUrl(e, debug) {
    const qs = new URLSearchParams(e.params || {}).toString();
    const base = debug ? e.wasm.replace(/\.html$/, '-ui.html') : e.wasm;
    return base + (qs ? '?' + qs : '');
  },
  playerUrl: (e, debug) => 'player.html?id=' + encodeURIComponent(e.id) + (debug ? '&dbg=1' : ''),
  // Minimal markdown: # ## ### headings, - lists, paragraphs, **bold**, *italic*, `code`, [text](url)
  md(src) {
    const inl = s => TE.esc(s)
      .replace(/`([^`]+)`/g, '<code>$1</code>')
      .replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>')
      .replace(/\*([^*]+)\*/g, '<em>$1</em>')
      .replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, (m, t, u) => /^(https?:|\/|\.|#)/.test(u) ? `<a href="${u}" target="_blank" rel="noopener">${t}</a>` : t);
    const out = []; let list = null, para = [];
    const flushP = () => { if (para.length) { out.push('<p>' + inl(para.join(' ')) + '</p>'); para = []; } };
    const flushL = () => { if (list) { out.push('<ul>' + list.map(li => '<li>' + inl(li) + '</li>').join('') + '</ul>'); list = null; } };
    for (const raw of (src || '').split('\n')) {
      const l = raw.trimEnd(), h = l.match(/^(#{1,3})\s+(.*)$/), li = l.match(/^\s*[-*]\s+(.*)$/);
      if (h) { flushP(); flushL(); const n = h[1].length + 1; out.push(`<h${n}>${inl(h[2])}</h${n}>`); }
      else if (li) { flushP(); (list = list || []).push(li[1]); }
      else if (!l.trim()) { flushP(); flushL(); }
      else { flushL(); para.push(l.trim()); }
    }
    flushP(); flushL();
    return out.join('');
  },
  copy(text) {
    if (navigator.clipboard && navigator.clipboard.writeText) return navigator.clipboard.writeText(text).catch(() => TE.copyFallback(text));
    return Promise.resolve(TE.copyFallback(text));
  },
  copyFallback(text) {
    const ta = document.createElement('textarea');
    ta.value = text; ta.style.cssText = 'position:fixed;opacity:0';
    document.body.appendChild(ta); ta.select();
    try { document.execCommand('copy'); } finally { ta.remove(); }
  },
};
