import{_ as s,o as n,c as e,a0 as p}from"./chunks/framework.BpJS36ta.js";const h=JSON.parse('{"title":"Array Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/array.md","filePath":"grammar/array.md"}'),t={name:"grammar/array.md"};function i(r,a,l,c,o,d){return n(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="array-grammar" tabindex="-1">Array Grammar <a class="header-anchor" href="#array-grammar" aria-label="Permalink to &quot;Array Grammar&quot;">​</a></h1><p>Grammar array membentuk node array dari element yang dipisahkan koma.</p><h2 id="empty-array" tabindex="-1">Empty array <a class="header-anchor" href="#empty-array" aria-label="Permalink to &quot;Empty array&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>[]</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  ArrayLiteral: (empty)</span></span></code></pre></div><h2 id="simple-array" tabindex="-1">Simple array <a class="header-anchor" href="#simple-array" aria-label="Permalink to &quot;Simple array&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>[1, 2, 3]</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  ArrayLiteral:</span></span>
<span class="line"><span>    Number: 1</span></span>
<span class="line"><span>    Number: 2</span></span>
<span class="line"><span>    Number: 3</span></span></code></pre></div><h2 id="array-with-expressions" tabindex="-1">Array with expressions <a class="header-anchor" href="#array-with-expressions" aria-label="Permalink to &quot;Array with expressions&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>[1 + 2, 3 * 4]</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  ArrayLiteral:</span></span>
<span class="line"><span>    Binary: +</span></span>
<span class="line"><span>      Left: Number: 1</span></span>
<span class="line"><span>      Right: Number: 2</span></span>
<span class="line"><span>    Binary: *</span></span>
<span class="line"><span>      Left: Number: 3</span></span>
<span class="line"><span>      Right: Number: 4</span></span></code></pre></div><h2 id="nested-array" tabindex="-1">Nested array <a class="header-anchor" href="#nested-array" aria-label="Permalink to &quot;Nested array&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>[1, [2, 3], 4]</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  ArrayLiteral:</span></span>
<span class="line"><span>    Number: 1</span></span>
<span class="line"><span>    ArrayLiteral:</span></span>
<span class="line"><span>      Number: 2</span></span>
<span class="line"><span>      Number: 3</span></span>
<span class="line"><span>    Number: 4</span></span></code></pre></div><h2 id="array-type" tabindex="-1">Array type <a class="header-anchor" href="#array-type" aria-label="Permalink to &quot;Array type&quot;">​</a></h2><p>Type array menggunakan <code>[]</code> sebagai postfix dari type element.</p><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x: number[] = [1, 2, 3]</span></span>
<span class="line"><span>empty: number[] = []</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Annotation:</span></span>
<span class="line"><span>    Name:</span></span>
<span class="line"><span>      Identifier: x</span></span>
<span class="line"><span>    Type:</span></span>
<span class="line"><span>      ArrayType:</span></span>
<span class="line"><span>        Identifier: number</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      ArrayLiteral:</span></span>
<span class="line"><span>        Number: 1</span></span>
<span class="line"><span>        Number: 2</span></span>
<span class="line"><span>        Number: 3</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  Annotation:</span></span>
<span class="line"><span>    Name:</span></span>
<span class="line"><span>      Identifier: empty</span></span>
<span class="line"><span>    Type:</span></span>
<span class="line"><span>      ArrayType:</span></span>
<span class="line"><span>        Identifier: number</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      ArrayLiteral: (empty)</span></span></code></pre></div><p>Nested array menggunakan <code>ArrayType</code> secara rekursif:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x: number[][] = [[1, 2], [3, 4]]</span></span></code></pre></div><p>AST type:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>ArrayType:</span></span>
<span class="line"><span>  ArrayType:</span></span>
<span class="line"><span>    Identifier: number</span></span></code></pre></div><p>Type array membatasi type setiap element.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x: number[] = [1, &quot;a&quot;]</span></span></code></pre></div><p><code>&quot;a&quot;</code> tidak sesuai dengan <code>number</code>, sehingga assignment ditolak oleh type checking.</p>`,35)])])}const m=s(t,[["render",i]]);export{h as __pageData,m as default};
