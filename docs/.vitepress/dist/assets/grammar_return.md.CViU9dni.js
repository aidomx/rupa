import{_ as n,o as s,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Return Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/return.md","filePath":"grammar/return.md"}'),t={name:"grammar/return.md"};function i(r,a,l,c,o,d){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="return-grammar" tabindex="-1">Return Grammar <a class="header-anchor" href="#return-grammar" aria-label="Permalink to &quot;Return Grammar&quot;">​</a></h1><p>Grammar return membentuk node return dari expression.</p><h2 id="return-with-value" tabindex="-1">Return with value <a class="header-anchor" href="#return-with-value" aria-label="Permalink to &quot;Return with value&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>return x + y</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Return:</span></span>
<span class="line"><span>  Binary: +</span></span>
<span class="line"><span>    Left: Identifier: x</span></span>
<span class="line"><span>    Right: Identifier: y</span></span></code></pre></div><h2 id="return-in-function" tabindex="-1">Return in function <a class="header-anchor" href="#return-in-function" aria-label="Permalink to &quot;Return in function&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x, y) {</span></span>
<span class="line"><span>    return x + y</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Function:</span></span>
<span class="line"><span>    Name: Identifier: add</span></span>
<span class="line"><span>    Parameters:</span></span>
<span class="line"><span>      Identifier: x</span></span>
<span class="line"><span>      Identifier: y</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Return:</span></span>
<span class="line"><span>        Binary: +</span></span>
<span class="line"><span>          Left: Identifier: x</span></span>
<span class="line"><span>          Right: Identifier: y</span></span></code></pre></div><h2 id="return-with-object" tabindex="-1">Return with object <a class="header-anchor" href="#return-with-object" aria-label="Permalink to &quot;Return with object&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>makePoint(px, py) {</span></span>
<span class="line"><span>    return { x: px, y: py }</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Function:</span></span>
<span class="line"><span>    Name: Identifier: makePoint</span></span>
<span class="line"><span>    Parameters:</span></span>
<span class="line"><span>      Identifier: px</span></span>
<span class="line"><span>      Identifier: py</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Return:</span></span>
<span class="line"><span>        Object:</span></span>
<span class="line"><span>          Entry 1:</span></span>
<span class="line"><span>            Key: Identifier: x</span></span>
<span class="line"><span>            Value: Identifier: px</span></span>
<span class="line"><span>          Entry 2:</span></span>
<span class="line"><span>            Key: Identifier: y</span></span>
<span class="line"><span>            Value: Identifier: py</span></span></code></pre></div>`,17)])])}const m=n(t,[["render",i]]);export{h as __pageData,m as default};
