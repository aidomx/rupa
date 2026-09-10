import{_ as s,o as n,c as e,a0 as p}from"./chunks/framework.BpJS36ta.js";const g=JSON.parse('{"title":"Assignment Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/assignment.md","filePath":"grammar/assignment.md"}'),i={name:"grammar/assignment.md"};function t(l,a,o,c,r,d){return n(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="assignment-grammar" tabindex="-1">Assignment Grammar <a class="header-anchor" href="#assignment-grammar" aria-label="Permalink to &quot;Assignment Grammar&quot;">​</a></h1><p>Grammar assignment membentuk node assignment dari target dan expression nilai.</p><h2 id="normal-assignment" tabindex="-1">Normal assignment <a class="header-anchor" href="#normal-assignment" aria-label="Permalink to &quot;Normal assignment&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>name = &quot;rupa&quot;</span></span>
<span class="line"><span>x = 1 + 2</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Assignment:</span></span>
<span class="line"><span>    Target:</span></span>
<span class="line"><span>      Identifier: name</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      String: &quot;rupa&quot;</span></span>
<span class="line"><span>  Assignment:</span></span>
<span class="line"><span>    Target:</span></span>
<span class="line"><span>      Identifier: x</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Binary: +</span></span>
<span class="line"><span>        Left: Number: 1</span></span>
<span class="line"><span>        Right: Number: 2</span></span></code></pre></div><p>Node utama dibuat sebagai assignment biasa.</p><h2 id="typed-assignment" tabindex="-1">Typed assignment <a class="header-anchor" href="#typed-assignment" aria-label="Permalink to &quot;Typed assignment&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>count: number = 2</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Assignment:</span></span>
<span class="line"><span>    Target:</span></span>
<span class="line"><span>      Identifier: count</span></span>
<span class="line"><span>    Type:</span></span>
<span class="line"><span>      Identifier: number</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Number: 2</span></span></code></pre></div><p>Type annotation menjadi child <code>Type</code> pada assignment.</p><h2 id="conditional-assignment" tabindex="-1">Conditional assignment <a class="header-anchor" href="#conditional-assignment" aria-label="Permalink to &quot;Conditional assignment&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>result ?= valid -&gt; &quot;Sukses&quot;</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Conditional Assignment:</span></span>
<span class="line"><span>    Target:</span></span>
<span class="line"><span>      Identifier: result</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Then:</span></span>
<span class="line"><span>        Condition:</span></span>
<span class="line"><span>          Literal ID: valid</span></span>
<span class="line"><span>        Result:</span></span>
<span class="line"><span>          String: &quot;Sukses&quot;</span></span></code></pre></div><p><code>?=</code> membentuk <code>NODE_CONDITIONAL_ASSIGN</code>. Expression di sebelah kanan tidak diparse sebagai grammar khusus conditional assignment; expression tersebut tetap diparse melalui grammar expression.</p><p>Karena itu hasil <code>?=</code> dapat berupa <code>NODE_THEN</code>, <code>NODE_FALLBACK</code>, binary expression, call, array, object, atau expression lain yang didukung parser.</p><h2 id="komposisi-dengan-then-dan-fallback" tabindex="-1">Komposisi dengan then dan fallback <a class="header-anchor" href="#komposisi-dengan-then-dan-fallback" aria-label="Permalink to &quot;Komposisi dengan then dan fallback&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>result ?= valid -&gt; primary | fallback | &quot;Unavailable&quot;</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Conditional Assignment:</span></span>
<span class="line"><span>    Target:</span></span>
<span class="line"><span>      Identifier: result</span></span>
<span class="line"><span>    Value:</span></span>
<span class="line"><span>      Then:</span></span>
<span class="line"><span>        Condition:</span></span>
<span class="line"><span>          Literal ID: valid</span></span>
<span class="line"><span>        Result:</span></span>
<span class="line"><span>          Fallback:</span></span>
<span class="line"><span>            Primary:</span></span>
<span class="line"><span>              Fallback:</span></span>
<span class="line"><span>                Primary: Literal ID: primary</span></span>
<span class="line"><span>                Fallback: Literal ID: fallback</span></span>
<span class="line"><span>            Fallback: String: &quot;Unavailable&quot;</span></span></code></pre></div><p>Ini menunjukkan tiga grammar berbeda:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>?=  → NODE_CONDITIONAL_ASSIGN</span></span>
<span class="line"><span>-&gt;  → NODE_THEN</span></span>
<span class="line"><span>|   → NODE_FALLBACK</span></span></code></pre></div>`,28)])])}const m=s(i,[["render",t]]);export{g as __pageData,m as default};
