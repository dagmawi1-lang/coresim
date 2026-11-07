#include "view.hpp"
#include <iomanip>
#include <sstream>

namespace coresim {

static std::string hex4(uint16_t v) { std::ostringstream o; o << std::hex << std::setw(4) << std::setfill('0') << v; return o.str(); }

std::string registerPanel(const State& b, const State& a, bool color) {
  const char* hi = color ? "\033[1;33m" : "";
  const char* rs = color ? "\033[0m" : "";
  std::ostringstream o;
  for (int i = 0; i < 8; ++i) {
    bool ch = a.r[i] != b.r[i];
    o << (ch ? hi : "") << "R" << i << "=" << std::setw(5) << a.r[i] << (ch ? "*" : " ") << rs << (i == 3 ? "\n" : "  ");
  }
  o << "\nPC=0x" << hex4(a.pc) << "  SP=0x" << hex4(a.sp) << "  flags[" << (a.f.z ? 'Z' : '-') << (a.f.n ? 'N' : '-') << (a.f.c ? 'C' : '-') << (a.f.v ? 'V' : '-') << "]  cycles=" << a.cycles;
  return o.str();
}

std::string listing(const Program& p) {
  std::ostringstream o;
  for (const auto& [addr, line] : p.sourceLine) {
    Instr in = Instr::decode(p.image[addr], p.image[addr + 1]);
    o << hex4(addr) << ":  " << hex4(in.hi()) << " " << hex4(in.imm) << "   " << std::left << std::setw(22) << disassemble(in) << " ; line " << line << ": " << p.source[line - 1] << "\n";
  }
  return o.str();
}

static std::string jsonEscape(const std::string& s) {
  std::ostringstream o;
  for (unsigned char c : s) {
    switch (c) { case '"': o << "\\\""; break; case '\\': o << "\\\\"; break; case '\n': o << "\\n"; break; case '<': o << "\\u003c"; break; case '>': o << "\\u003e"; break; case '&': o << "\\u0026"; break;
      default: if (c < 0x20) o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c) << std::dec; else o << c; }
  }
  return o.str();
}

std::string htmlTrace(const Program& p, const std::vector<TraceEntry>& trace, const std::vector<uint16_t>& output, const std::string& title) {
  std::ostringstream js;
  js << "{\"title\":\"" << jsonEscape(title) << "\",\"source\":[";
  for (size_t i = 0; i < p.source.size(); ++i) js << (i ? "," : "") << "\"" << jsonEscape(p.source[i]) << "\"";
  js << "],\"output\":[";
  for (size_t i = 0; i < output.size(); ++i) js << (i ? "," : "") << output[i];
  js << "],\"steps\":[";
  auto st = [&](const State& s) {
    std::ostringstream o; o << "{\"r\":[";
    for (int i = 0; i < 8; ++i) o << (i ? "," : "") << s.r[i];
    o << "],\"pc\":" << s.pc << ",\"sp\":" << s.sp << ",\"f\":\"" << (s.f.z ? 'Z' : '-') << (s.f.n ? 'N' : '-') << (s.f.c ? 'C' : '-') << (s.f.v ? 'V' : '-') << "\",\"cy\":" << s.cycles << "}";
    return o.str();
  };
  for (size_t i = 0; i < trace.size(); ++i) {
    const auto& t = trace[i];
    auto it = p.sourceLine.find(t.before.pc);
    js << (i ? "," : "") << "{\"line\":" << (it == p.sourceLine.end() ? 0 : it->second) << ",\"text\":\"" << jsonEscape(t.text) << "\",\"before\":" << st(t.before) << ",\"after\":" << st(t.after) << ",\"w\":[";
    for (size_t k = 0; k < t.memWrites.size(); ++k) js << (k ? "," : "") << "[" << t.memWrites[k].first << "," << t.memWrites[k].second << "]";
    js << "]}";
  }
  js << "]}";
  return R"HTML(<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>CoreSim trace</title>
<style>body{margin:0;font-family:system-ui,sans-serif;background:#0d1117;color:#e6edf3}main{max-width:1100px;margin:auto;padding:22px}h1{margin:0 0 4px;font-size:22px}.muted{color:#8b949e}
.grid{display:grid;grid-template-columns:1.2fr 1fr;gap:16px;margin-top:14px}.card{background:#161b22;border:1px solid #30363d;border-radius:12px;padding:14px}
pre{margin:0;font:13px/1.5 ui-monospace,Menlo,monospace;max-height:460px;overflow:auto}.ln{display:block;padding:0 6px;border-radius:4px}.ln.cur{background:#1f6feb55;outline:1px solid #1f6feb}
.regs{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}.reg{background:#0d1117;border:1px solid #30363d;border-radius:8px;padding:8px;font:13px ui-monospace,monospace}.reg b{display:block;color:#8b949e;font-size:11px}.reg.ch{border-color:#d29922;background:#d2992222}
.ctl{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:12px}button{background:#238636;color:#fff;border:0;border-radius:8px;padding:8px 12px;font-weight:700;cursor:pointer}button.sec{background:#30363d}input[type=range]{flex:1;min-width:160px}
.flags span{display:inline-block;width:28px;text-align:center;border-radius:6px;margin-right:4px;background:#30363d;padding:4px 0;font:12px ui-monospace,monospace}.flags span.on{background:#1f6feb}
.kv{font:13px ui-monospace,monospace;margin-top:10px}.instr{font:16px ui-monospace,monospace;color:#79c0ff;margin:8px 0}@media(max-width:800px){.grid{grid-template-columns:1fr}}</style></head>
<body><main><h1 id="title"></h1><div class="muted">CoreSim · CS16 CPU · step through the program and watch the registers change</div>
<div class="ctl"><button class="sec" id="first">⏮</button><button class="sec" id="prev">◀ Back</button><button id="next">Step ▶</button><button class="sec" id="play">▶ Play</button><input type="range" id="slider" min="0" value="0" aria-label="Step"><span id="pos" class="muted"></span></div>
<div class="grid"><div class="card"><div class="muted">Source</div><pre id="src"></pre></div>
<div class="card"><div class="muted">Executing</div><div class="instr" id="instr"></div><div class="regs" id="regs"></div>
<div class="kv" id="pcsp"></div><div class="flags kv" id="flags"></div><div class="kv" id="mem"></div><div class="kv" id="out"></div></div></div></main>
<script>const D=)HTML" + js.str() + R"HTML(;
const $=id=>document.getElementById(id);let i=0,timer=null;$('title').textContent=D.title;$('slider').max=Math.max(0,D.steps.length-1);
$('src').innerHTML=D.source.map((l,k)=>`<span class="ln" id="L${k+1}">${String(k+1).padStart(3)}  ${l.replace(/&/g,'&amp;').replace(/</g,'&lt;')}</span>`).join('');
function show(){const s=D.steps[i];if(!s)return;const a=s.after,b=s.before;$('instr').textContent=`0x${b.pc.toString(16).padStart(4,'0')}: ${s.text}`;
$('regs').innerHTML=a.r.map((v,k)=>`<div class="reg ${v!==b.r[k]?'ch':''}"><b>R${k}</b>${v} <span class="muted">0x${v.toString(16).padStart(4,'0')}</span></div>`).join('');
$('pcsp').textContent=`PC 0x${a.pc.toString(16).padStart(4,'0')}   SP 0x${a.sp.toString(16).padStart(4,'0')}   cycles ${a.cy}`;
$('flags').innerHTML='Flags '+['Z','N','C','V'].map((f,k)=>`<span class="${a.f[k]!=='-'?'on':''}" title="${{Z:'zero',N:'negative',C:'carry/borrow',V:'signed overflow'}[f]}">${f}</span>`).join('');
$('mem').textContent=s.w.length?'Memory writes: '+s.w.map(([ad,v])=>`[0x${ad.toString(16)}]=${v}`).join(', '):'';
const outSoFar=D.steps.slice(0,i+1).filter(x=>x.text.startsWith('OUT')).map(x=>x.after.r[+x.text.match(/R(\d)/)[1]]);$('out').textContent='Output: '+(outSoFar.join(', ')||'—');
document.querySelectorAll('.ln.cur').forEach(e=>e.classList.remove('cur'));const L=$('L'+s.line);if(L){L.classList.add('cur');L.scrollIntoView({block:'nearest'})}
$('slider').value=i;$('pos').textContent=`step ${i+1} / ${D.steps.length}`}
const go=n=>{i=Math.max(0,Math.min(D.steps.length-1,n));show()};$('next').onclick=()=>go(i+1);$('prev').onclick=()=>go(i-1);$('first').onclick=()=>go(0);$('slider').oninput=e=>go(+e.target.value);
$('play').onclick=()=>{if(timer){clearInterval(timer);timer=null;$('play').textContent='▶ Play';return}$('play').textContent='⏸ Pause';timer=setInterval(()=>{if(i>=D.steps.length-1){clearInterval(timer);timer=null;$('play').textContent='▶ Play'}else go(i+1)},250)};
document.addEventListener('keydown',e=>{if(e.key==='ArrowRight')go(i+1);if(e.key==='ArrowLeft')go(i-1)});show();</script></body></html>)HTML";
}

}  // namespace coresim
