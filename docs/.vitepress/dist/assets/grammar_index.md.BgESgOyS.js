import{_ as n,o as s,c as e,a0 as l}from"./chunks/framework.CShDTZbC.js";const d=JSON.parse('{"title":"Rupa Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/index.md","filePath":"grammar/index.md"}'),t={name:"grammar/index.md"};function i(r,a,p,o,m,c){return s(),e("div",null,[...a[0]||(a[0]=[l(`<h1 id="rupa-grammar" tabindex="-1">Rupa Grammar <a class="header-anchor" href="#rupa-grammar" aria-label="Permalink to &quot;Rupa Grammar&quot;">​</a></h1><p>Dokumentasi ini menjelaskan grammar Rupa dari sudut pandang parser dan AST.</p><p><code>docs/syntax.md</code> menjelaskan cara memakai syntax. Sebaliknya, direktori ini menjelaskan bagaimana bentuk source yang sudah dikenali parser direpresentasikan sebagai node dan hubungan antar-node.</p><p>Alur umum:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>source</span></span>
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
<span class="line"><span>└── member expression</span></span></code></pre></div><p><code>?=</code>, <code>-&gt;</code>, dan <code>|</code> tetap dipandang sebagai grammar yang berbeda. Mereka dapat muncul dalam satu AST karena grammar expression dapat dikomposisikan.</p><h2 id="navigasi" tabindex="-1">Navigasi <a class="header-anchor" href="#navigasi" aria-label="Permalink to &quot;Navigasi&quot;">​</a></h2><h3 id="dasar" tabindex="-1">Dasar <a class="header-anchor" href="#dasar" aria-label="Permalink to &quot;Dasar&quot;">​</a></h3><ul><li><a href="./grammar.html">Grammar</a> - Overview grammar</li><li><a href="./literal.html">Literal</a> - Nilai konstan</li><li><a href="./expression.html">Expression</a> - Kombinasi value dan operator</li><li><a href="./assignment.html">Assignment</a> - Assignment grammar</li><li><a href="./update.html">Update</a> - Update operator</li><li><a href="./fallback.html">Fallback</a> - Conditional assignment</li></ul><h3 id="data" tabindex="-1">Data <a class="header-anchor" href="#data" aria-label="Permalink to &quot;Data&quot;">​</a></h3><ul><li><a href="./array.html">Array</a> - Array grammar</li><li><a href="./object.html">Object</a> - Object grammar</li><li><a href="./struct.html">Struct</a> - Struct grammar</li><li><a href="./member.html">Member</a> - Member access</li></ul><h3 id="function" tabindex="-1">Function <a class="header-anchor" href="#function" aria-label="Permalink to &quot;Function&quot;">​</a></h3><ul><li><a href="./function.html">Function</a> - Function grammar</li><li><a href="./call.html">Call</a> - Function call</li><li><a href="./return.html">Return</a> - Return statement</li></ul><h3 id="control-flow" tabindex="-1">Control Flow <a class="header-anchor" href="#control-flow" aria-label="Permalink to &quot;Control Flow&quot;">​</a></h3><ul><li><a href="./if.html">If</a> - If statement</li><li><a href="./case.html">Case</a> - Pattern matching</li><li><a href="./loop.html">Loop</a> - Loop statement</li><li><a href="./control.html">Control</a> - Break/continue</li><li><a href="./block.html">Block</a> - Block statement</li></ul><h3 id="asynchronous" tabindex="-1">Asynchronous <a class="header-anchor" href="#asynchronous" aria-label="Permalink to &quot;Asynchronous&quot;">​</a></h3><ul><li><a href="./async.html">Async</a> - Async statement</li><li><a href="./then.html">Then</a> - Then expression</li></ul><h3 id="program" tabindex="-1">Program <a class="header-anchor" href="#program" aria-label="Permalink to &quot;Program&quot;">​</a></h3><ul><li><a href="./module.html">Module</a> - Module import</li><li><a href="./annotation.html">Annotation</a> - Type annotation</li><li><a href="./print.html">Print</a> - Print statement</li></ul>`,23)])])}const u=n(t,[["render",i]]);export{d as __pageData,u as default};
