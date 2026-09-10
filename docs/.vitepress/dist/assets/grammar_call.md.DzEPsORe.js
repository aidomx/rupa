import{_ as s,o as n,c as e,a0 as p}from"./chunks/framework.BpJS36ta.js";const u=JSON.parse('{"title":"Call Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/call.md","filePath":"grammar/call.md"}'),l={name:"grammar/call.md"};function i(t,a,r,c,o,d){return n(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="call-grammar" tabindex="-1">Call Grammar <a class="header-anchor" href="#call-grammar" aria-label="Permalink to &quot;Call Grammar&quot;">​</a></h1><p>Grammar call membentuk node call dari callee dan arguments.</p><h2 id="simple-call" tabindex="-1">Simple call <a class="header-anchor" href="#simple-call" aria-label="Permalink to &quot;Simple call&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(1, 2)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Call:</span></span>
<span class="line"><span>    Callee:</span></span>
<span class="line"><span>      Identifier: add</span></span>
<span class="line"><span>    Arg 1:</span></span>
<span class="line"><span>      Number: 1</span></span>
<span class="line"><span>    Arg 2:</span></span>
<span class="line"><span>      Number: 2</span></span></code></pre></div><h2 id="nested-call" tabindex="-1">Nested call <a class="header-anchor" href="#nested-call" aria-label="Permalink to &quot;Nested call&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>double(triple(5))</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Call:</span></span>
<span class="line"><span>    Callee:</span></span>
<span class="line"><span>      Identifier: double</span></span>
<span class="line"><span>    Arg 1:</span></span>
<span class="line"><span>      Call:</span></span>
<span class="line"><span>        Callee:</span></span>
<span class="line"><span>          Identifier: triple</span></span>
<span class="line"><span>        Arg 1:</span></span>
<span class="line"><span>          Number: 5</span></span></code></pre></div><h2 id="call-with-expression-args" tabindex="-1">Call with expression args <a class="header-anchor" href="#call-with-expression-args" aria-label="Permalink to &quot;Call with expression args&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&quot;result:&quot;, x + y, true)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Call:</span></span>
<span class="line"><span>    Callee:</span></span>
<span class="line"><span>      Identifier: print</span></span>
<span class="line"><span>    Arg 1:</span></span>
<span class="line"><span>      String: result:</span></span>
<span class="line"><span>    Arg 2:</span></span>
<span class="line"><span>      Binary: +</span></span>
<span class="line"><span>        Left: Identifier: x</span></span>
<span class="line"><span>        Right: Identifier: y</span></span>
<span class="line"><span>    Arg 3:</span></span>
<span class="line"><span>      Boolean: true</span></span></code></pre></div>`,17)])])}const g=s(l,[["render",i]]);export{u as __pageData,g as default};
