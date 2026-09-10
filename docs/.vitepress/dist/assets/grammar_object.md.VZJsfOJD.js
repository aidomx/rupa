import{_ as s,o as n,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const b=JSON.parse('{"title":"Object Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/object.md","filePath":"grammar/object.md"}'),t={name:"grammar/object.md"};function i(l,a,c,r,o,d){return n(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="object-grammar" tabindex="-1">Object Grammar <a class="header-anchor" href="#object-grammar" aria-label="Permalink to &quot;Object Grammar&quot;">​</a></h1><p>Grammar object membentuk node object dari key-value pairs.</p><h2 id="simple-object" tabindex="-1">Simple object <a class="header-anchor" href="#simple-object" aria-label="Permalink to &quot;Simple object&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>person = { name: &quot;Rupa&quot;, age: 1 }</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Assignment:</span></span>
<span class="line"><span>    Target: Identifier: person</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Entry 1:</span></span>
<span class="line"><span>          Key: Identifier: name</span></span>
<span class="line"><span>          Value: String: Rupa</span></span>
<span class="line"><span>        Entry 2:</span></span>
<span class="line"><span>          Key: Identifier: age</span></span>
<span class="line"><span>          Value: Number: 1</span></span></code></pre></div><h2 id="nested-object" tabindex="-1">Nested object <a class="header-anchor" href="#nested-object" aria-label="Permalink to &quot;Nested object&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>company = { ceo: { name: &quot;Boss&quot;, title: &quot;CEO&quot; } }</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Assignment:</span></span>
<span class="line"><span>    Target: Identifier: company</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Entry 1:</span></span>
<span class="line"><span>          Key: Identifier: ceo</span></span>
<span class="line"><span>          Value:</span></span>
<span class="line"><span>            Object:</span></span>
<span class="line"><span>              Entry 1:</span></span>
<span class="line"><span>                Key: Identifier: name</span></span>
<span class="line"><span>                Value: String: Boss</span></span>
<span class="line"><span>              Entry 2:</span></span>
<span class="line"><span>                Key: Identifier: title</span></span>
<span class="line"><span>                Value: String: CEO</span></span></code></pre></div><h2 id="object-with-array" tabindex="-1">Object with array <a class="header-anchor" href="#object-with-array" aria-label="Permalink to &quot;Object with array&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>team = { members: [&quot;Alice&quot;, &quot;Bob&quot;, &quot;Charlie&quot;] }</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Assignment:</span></span>
<span class="line"><span>    Target: Identifier: team</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Entry 1:</span></span>
<span class="line"><span>          Key: Identifier: members</span></span>
<span class="line"><span>          Value:</span></span>
<span class="line"><span>            Array:</span></span>
<span class="line"><span>              String: Alice</span></span>
<span class="line"><span>              String: Bob</span></span>
<span class="line"><span>              String: Charlie</span></span></code></pre></div>`,17)])])}const h=s(t,[["render",i]]);export{b as __pageData,h as default};
