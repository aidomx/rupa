import{_ as s,o as n,c as p,a0 as e}from"./chunks/framework.BpJS36ta.js";const m=JSON.parse('{"title":"Struct Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/struct.md","filePath":"grammar/struct.md"}'),t={name:"grammar/struct.md"};function i(l,a,r,c,o,u){return n(),p("div",null,[...a[0]||(a[0]=[e(`<h1 id="struct-grammar" tabindex="-1">Struct Grammar <a class="header-anchor" href="#struct-grammar" aria-label="Permalink to &quot;Struct Grammar&quot;">​</a></h1><p>Grammar struct membentuk node struct dari nama dan field declarations.</p><h2 id="simple-struct" tabindex="-1">Simple struct <a class="header-anchor" href="#simple-struct" aria-label="Permalink to &quot;Simple struct&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>People {</span></span>
<span class="line"><span>    name: string</span></span>
<span class="line"><span>    age: number</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Struct:</span></span>
<span class="line"><span>    Name: Identifier: People</span></span>
<span class="line"><span>    Body:</span></span>
<span class="line"><span>      Block:</span></span>
<span class="line"><span>        Annotation:</span></span>
<span class="line"><span>          Name: Identifier: name</span></span>
<span class="line"><span>          Type: Identifier: string</span></span>
<span class="line"><span>        Annotation:</span></span>
<span class="line"><span>          Name: Identifier: age</span></span>
<span class="line"><span>          Type: Identifier: number</span></span></code></pre></div><h2 id="struct-usage" tabindex="-1">Struct usage <a class="header-anchor" href="#struct-usage" aria-label="Permalink to &quot;Struct usage&quot;">​</a></h2><p>Struct berfungsi sebagai blueprint untuk membuat object:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>People {</span></span>
<span class="line"><span>    name: string</span></span>
<span class="line"><span>    age: number</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>person = { name: &quot;Rupa&quot;, age: 20 }</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Struct:</span></span>
<span class="line"><span>    Name: Identifier: People</span></span>
<span class="line"><span>    Body:</span></span>
<span class="line"><span>      Block:</span></span>
<span class="line"><span>        Annotation:</span></span>
<span class="line"><span>          Name: Identifier: name</span></span>
<span class="line"><span>          Type: Identifier: string</span></span>
<span class="line"><span>        Annotation:</span></span>
<span class="line"><span>          Name: Identifier: age</span></span>
<span class="line"><span>          Type: Identifier: number</span></span>
<span class="line"><span>  Assignment:</span></span>
<span class="line"><span>    Target: Identifier: person</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Entry 1:</span></span>
<span class="line"><span>          Key: Identifier: name</span></span>
<span class="line"><span>          Value: String: Rupa</span></span>
<span class="line"><span>        Entry 2:</span></span>
<span class="line"><span>          Key: Identifier: age</span></span>
<span class="line"><span>          Value: Number: 20</span></span></code></pre></div>`,12)])])}const g=s(t,[["render",i]]);export{m as __pageData,g as default};
