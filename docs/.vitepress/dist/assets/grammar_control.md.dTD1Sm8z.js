import{_ as n,o as s,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const u=JSON.parse('{"title":"Control Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/control.md","filePath":"grammar/control.md"}'),t={name:"grammar/control.md"};function i(l,a,o,r,c,d){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="control-grammar" tabindex="-1">Control Grammar <a class="header-anchor" href="#control-grammar" aria-label="Permalink to &quot;Control Grammar&quot;">​</a></h1><p>Grammar control membentuk node break dan continue.</p><h2 id="break" tabindex="-1">Break <a class="header-anchor" href="#break" aria-label="Permalink to &quot;Break&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>break</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Break</span></span></code></pre></div><h2 id="continue" tabindex="-1">Continue <a class="header-anchor" href="#continue" aria-label="Permalink to &quot;Continue&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>continue</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Continue</span></span></code></pre></div><h2 id="usage-in-loop" tabindex="-1">Usage in loop <a class="header-anchor" href="#usage-in-loop" aria-label="Permalink to &quot;Usage in loop&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>for i &lt; 10 {</span></span>
<span class="line"><span>    if i == 3: continue</span></span>
<span class="line"><span>    if i == 7: break</span></span>
<span class="line"><span>    print(i)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Loop: for</span></span>
<span class="line"><span>    Condition: Binary: &lt;</span></span>
<span class="line"><span>      Left: Identifier: i</span></span>
<span class="line"><span>      Right: Number: 10</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      If:</span></span>
<span class="line"><span>        Condition: Binary: ==</span></span>
<span class="line"><span>          Left: Identifier: i</span></span>
<span class="line"><span>          Right: Number: 3</span></span>
<span class="line"><span>        Body: Continue</span></span>
<span class="line"><span>      If:</span></span>
<span class="line"><span>        Condition: Binary: ==</span></span>
<span class="line"><span>          Left: Identifier: i</span></span>
<span class="line"><span>          Right: Number: 7</span></span>
<span class="line"><span>        Body: Break</span></span>
<span class="line"><span>      Print: Identifier: i</span></span></code></pre></div>`,17)])])}const g=n(t,[["render",i]]);export{u as __pageData,g as default};
