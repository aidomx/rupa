import{_ as n,o as s,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const u=JSON.parse('{"title":"Rupa Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/grammar.md","filePath":"grammar/grammar.md"}'),i={name:"grammar/grammar.md"};function t(l,a,r,c,m,o){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="rupa-grammar" tabindex="-1">Rupa Grammar <a class="header-anchor" href="#rupa-grammar" aria-label="Permalink to &quot;Rupa Grammar&quot;">​</a></h1><p>Dokumentasi ini menjelaskan grammar Rupa dari sudut pandang parser dan AST.</p><p><code>docs/syntax.md</code> menjelaskan cara memakai syntax. Sebaliknya, direktori ini menjelaskan bagaimana bentuk source yang sudah dikenali parser direpresentasikan sebagai node dan hubungan antar-node.</p><p>Alur umum:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>source</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>token</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>grammarParseStatement()</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>grammarParseExpr()</span></span>
<span class="line"><span>  ↓</span></span>
<span class="line"><span>AST node</span></span></code></pre></div><h2 id="status-dokumentasi" tabindex="-1">Status dokumentasi <a class="header-anchor" href="#status-dokumentasi" aria-label="Permalink to &quot;Status dokumentasi&quot;">​</a></h2><p>Dokumentasi ini mengikuti source dan test yang tersedia saat ini. Bentuk yang belum diuji atau belum direpresentasikan oleh AST tidak dianggap sebagai grammar yang sudah dikunci.</p><h2 id="peta-grammar" tabindex="-1">Peta grammar <a class="header-anchor" href="#peta-grammar" aria-label="Permalink to &quot;Peta grammar&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>statement</span></span>
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
<span class="line"><span>└── member expression</span></span></code></pre></div><p><code>?=</code>, <code>-&gt;</code>, dan <code>|</code> tetap dipandang sebagai grammar yang berbeda. Mereka dapat muncul dalam satu AST karena grammar expression dapat dikomposisikan.</p>`,10)])])}const g=n(i,[["render",t]]);export{u as __pageData,g as default};
