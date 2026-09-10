import{_ as n,o as s,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Annotation Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/annotation.md","filePath":"grammar/annotation.md"}'),t={name:"grammar/annotation.md"};function i(l,a,o,r,c,d){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="annotation-grammar" tabindex="-1">Annotation Grammar <a class="header-anchor" href="#annotation-grammar" aria-label="Permalink to &quot;Annotation Grammar&quot;">​</a></h1><p>Grammar annotation membentuk node annotation dari nama dan type.</p><h2 id="simple-annotation" tabindex="-1">Simple annotation <a class="header-anchor" href="#simple-annotation" aria-label="Permalink to &quot;Simple annotation&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>name: string</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Annotation:</span></span>
<span class="line"><span>    Name:</span></span>
<span class="line"><span>      Identifier: name</span></span>
<span class="line"><span>    Type:</span></span>
<span class="line"><span>      Identifier: string</span></span></code></pre></div><h2 id="annotation-with-value" tabindex="-1">Annotation with value <a class="header-anchor" href="#annotation-with-value" aria-label="Permalink to &quot;Annotation with value&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x: number = 1</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Annotation:</span></span>
<span class="line"><span>    Name:</span></span>
<span class="line"><span>      Identifier: x</span></span>
<span class="line"><span>    Type:</span></span>
<span class="line"><span>      Identifier: number</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Number: 1</span></span></code></pre></div><h2 id="multiple-annotations" tabindex="-1">Multiple annotations <a class="header-anchor" href="#multiple-annotations" aria-label="Permalink to &quot;Multiple annotations&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>count: number = 42</span></span>
<span class="line"><span>name: string = &quot;hello&quot;</span></span>
<span class="line"><span>active: boolean = true</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Annotation:</span></span>
<span class="line"><span>    Name: Identifier: count</span></span>
<span class="line"><span>    Type: Identifier: number</span></span>
<span class="line"><span>    Value: Number: 42</span></span>
<span class="line"><span>  Annotation:</span></span>
<span class="line"><span>    Name: Identifier: name</span></span>
<span class="line"><span>    Type: Identifier: string</span></span>
<span class="line"><span>    Value: String: hello</span></span>
<span class="line"><span>  Annotation:</span></span>
<span class="line"><span>    Name: Identifier: active</span></span>
<span class="line"><span>    Type: Identifier: boolean</span></span>
<span class="line"><span>    Value: Boolean: true</span></span></code></pre></div><h2 id="array-type-annotation" tabindex="-1">Array type annotation <a class="header-anchor" href="#array-type-annotation" aria-label="Permalink to &quot;Array type annotation&quot;">​</a></h2><p>Array type menggunakan <code>[]</code> setelah type dasar.</p><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x: number[] = [1, 2, 3]</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
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
<span class="line"><span>        Number: 3</span></span></code></pre></div>`,23)])])}const m=n(t,[["render",i]]);export{h as __pageData,m as default};
