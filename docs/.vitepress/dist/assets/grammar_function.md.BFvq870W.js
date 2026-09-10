import{_ as n,o as s,c as e,a0 as p}from"./chunks/framework.BpJS36ta.js";const m=JSON.parse('{"title":"Function Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/function.md","filePath":"grammar/function.md"}'),i={name:"grammar/function.md"};function t(l,a,c,r,o,d){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="function-grammar" tabindex="-1">Function Grammar <a class="header-anchor" href="#function-grammar" aria-label="Permalink to &quot;Function Grammar&quot;">​</a></h1><p>Function declaration dan function call menggunakan pola awal yang sama: identifier diikuti parentheses. Parser membedakan declaration ketika bentuk tersebut diikuti body block.</p><h2 id="declaration" tabindex="-1">Declaration <a class="header-anchor" href="#declaration" aria-label="Permalink to &quot;Declaration&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x: number, y: number) {</span></span>
<span class="line"><span>    return x + y</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Function:</span></span>
<span class="line"><span>    Name:</span></span>
<span class="line"><span>      Identifier: add</span></span>
<span class="line"><span>    Parameters:</span></span>
<span class="line"><span>      Annotation:</span></span>
<span class="line"><span>        Name: Identifier: x</span></span>
<span class="line"><span>        Type: Identifier: number</span></span>
<span class="line"><span>      Annotation:</span></span>
<span class="line"><span>        Name: Identifier: y</span></span>
<span class="line"><span>        Type: Identifier: number</span></span>
<span class="line"><span>    Body:</span></span>
<span class="line"><span>      Block:</span></span>
<span class="line"><span>        Return:</span></span>
<span class="line"><span>          Binary: +</span></span>
<span class="line"><span>            Identifier: x</span></span>
<span class="line"><span>            Identifier: y</span></span></code></pre></div><h2 id="call-statement" tabindex="-1">Call statement <a class="header-anchor" href="#call-statement" aria-label="Permalink to &quot;Call statement&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(1, 2)</span></span></code></pre></div><p>Jika pola identifier + arguments tidak membentuk declaration dengan body, parser membuat <code>Call</code>:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Call:</span></span>
<span class="line"><span>    Callee:</span></span>
<span class="line"><span>      Identifier: add</span></span>
<span class="line"><span>    Arg 1:</span></span>
<span class="line"><span>      Number: 1</span></span>
<span class="line"><span>    Arg 2:</span></span>
<span class="line"><span>      Number: 2</span></span></code></pre></div><h2 id="function-with-object-return" tabindex="-1">Function with object return <a class="header-anchor" href="#function-with-object-return" aria-label="Permalink to &quot;Function with object return&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>makePoint(px, py) {</span></span>
<span class="line"><span>    return { x: px, y: py }</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Function:</span></span>
<span class="line"><span>    Name:</span></span>
<span class="line"><span>      Identifier: makePoint</span></span>
<span class="line"><span>    Parameters:</span></span>
<span class="line"><span>      Identifier: px</span></span>
<span class="line"><span>      Identifier: py</span></span>
<span class="line"><span>    Body:</span></span>
<span class="line"><span>      Block:</span></span>
<span class="line"><span>        Return:</span></span>
<span class="line"><span>          Object:</span></span>
<span class="line"><span>            Entry 1:</span></span>
<span class="line"><span>              Key: Identifier: x</span></span>
<span class="line"><span>              Value: Identifier: px</span></span>
<span class="line"><span>            Entry 2:</span></span>
<span class="line"><span>              Key: Identifier: y</span></span>
<span class="line"><span>              Value: Identifier: py</span></span></code></pre></div>`,15)])])}const h=n(i,[["render",t]]);export{m as __pageData,h as default};
