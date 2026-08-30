import{_ as n,o as s,c as e,a2 as p}from"./chunks/framework.BXzK3EA4.js";const d=JSON.parse('{"title":"Rupa Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/grammar.md","filePath":"grammar/grammar.md"}'),r={name:"grammar/grammar.md"};function l(i,a,t,c,m,u){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="rupa-grammar" tabindex="-1">Rupa Grammar <a class="header-anchor" href="#rupa-grammar" aria-label="Permalink to &quot;Rupa Grammar&quot;">​</a></h1><p>Dokumentasi ini menjelaskan grammar Rupa dari sudut pandang parser dan AST.</p><p><code>docs/syntax.md</code> menjelaskan cara memakai syntax. Sebaliknya, direktori ini menjelaskan bagaimana bentuk source yang sudah dikenali parser direpresentasikan sebagai node dan hubungan antar-node.</p><p>Alur umum:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>source</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>token</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>grammarParseStatement()</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>grammarParseExpr()</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>AST node</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br></div></div><h2 id="status-dokumentasi" tabindex="-1">Status dokumentasi <a class="header-anchor" href="#status-dokumentasi" aria-label="Permalink to &quot;Status dokumentasi&quot;">​</a></h2><p>Dokumentasi ini mengikuti source dan test yang tersedia saat ini. Bentuk yang belum diuji atau belum direpresentasikan oleh AST tidak dianggap sebagai grammar yang sudah dikunci.</p><h2 id="peta-grammar" tabindex="-1">Peta grammar <a class="header-anchor" href="#peta-grammar" aria-label="Permalink to &quot;Peta grammar&quot;">​</a></h2><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>statement</span></span>
<span class="line"><span>├── annotation</span></span>
<span class="line"><span>├── assignment</span></span>
<span class="line"><span>│   ├── normal assignment</span></span>
<span class="line"><span>│   └── conditional assignment</span></span>
<span class="line"><span>├── function / call</span></span>
<span class="line"><span>├── struct</span></span>
<span class="line"><span>├── if / elseif / else</span></span>
<span class="line"><span>├── loop</span></span>
<span class="line"><span>├── print</span></span>
<span class="line"><span>├── return</span></span>
<span class="line"><span>├── break / continue</span></span>
<span class="line"><span>├── module statement</span></span>
<span class="line"><span>├── update</span></span>
<span class="line"><span>└── expression statement</span></span>
<span class="line"><span></span></span>
<span class="line"><span>expression</span></span>
<span class="line"><span>├── literal / identifier</span></span>
<span class="line"><span>├── binary expression</span></span>
<span class="line"><span>├── then</span></span>
<span class="line"><span>├── fallback</span></span>
<span class="line"><span>├── array literal</span></span>
<span class="line"><span>├── object literal</span></span>
<span class="line"><span>├── call expression</span></span>
<span class="line"><span>└── member expression</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br><span class="line-number">10</span><br><span class="line-number">11</span><br><span class="line-number">12</span><br><span class="line-number">13</span><br><span class="line-number">14</span><br><span class="line-number">15</span><br><span class="line-number">16</span><br><span class="line-number">17</span><br><span class="line-number">18</span><br><span class="line-number">19</span><br><span class="line-number">20</span><br><span class="line-number">21</span><br><span class="line-number">22</span><br><span class="line-number">23</span><br><span class="line-number">24</span><br><span class="line-number">25</span><br></div></div><p><code>?=</code>, <code>-&gt;</code>, dan <code>|</code> tetap dipandang sebagai grammar yang berbeda. Mereka dapat muncul dalam satu AST karena grammar expression dapat dikomposisikan.</p>`,10)])])}const o=n(r,[["render",l]]);export{d as __pageData,o as default};
