import{_ as n,o as s,c as p,a0 as e}from"./chunks/framework.BpJS36ta.js";const u=JSON.parse('{"title":"Loop Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/loop.md","filePath":"grammar/loop.md"}'),i={name:"grammar/loop.md"};function l(t,a,o,c,r,d){return s(),p("div",null,[...a[0]||(a[0]=[e(`<h1 id="loop-grammar" tabindex="-1">Loop Grammar <a class="header-anchor" href="#loop-grammar" aria-label="Permalink to &quot;Loop Grammar&quot;">​</a></h1><p>Grammar loop membentuk node loop dari kind, condition, dan body.</p><h2 id="for-loop" tabindex="-1">For loop <a class="header-anchor" href="#for-loop" aria-label="Permalink to &quot;For loop&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>for i &lt; 10 {</span></span>
<span class="line"><span>    print(i)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Loop: for</span></span>
<span class="line"><span>    Condition: Binary: &lt;</span></span>
<span class="line"><span>      Left: Identifier: i</span></span>
<span class="line"><span>      Right: Number: 10</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print:</span></span>
<span class="line"><span>        Identifier: i</span></span></code></pre></div><h2 id="rev-loop" tabindex="-1">Rev loop <a class="header-anchor" href="#rev-loop" aria-label="Permalink to &quot;Rev loop&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>rev i &gt; 0 {</span></span>
<span class="line"><span>    print(i)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Loop: rev</span></span>
<span class="line"><span>    Condition: Binary: &gt;</span></span>
<span class="line"><span>      Left: Identifier: i</span></span>
<span class="line"><span>      Right: Number: 0</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print:</span></span>
<span class="line"><span>        Identifier: i</span></span></code></pre></div><h2 id="while-loop" tabindex="-1">While loop <a class="header-anchor" href="#while-loop" aria-label="Permalink to &quot;While loop&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>while x &lt; 10 {</span></span>
<span class="line"><span>    print(x)</span></span>
<span class="line"><span>    x++</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Loop: while</span></span>
<span class="line"><span>    Condition: Binary: &lt;</span></span>
<span class="line"><span>      Left: Identifier: x</span></span>
<span class="line"><span>      Right: Number: 10</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print:</span></span>
<span class="line"><span>        Identifier: x</span></span>
<span class="line"><span>      Update: postfix ++</span></span>
<span class="line"><span>        Identifier: x</span></span></code></pre></div><h2 id="loop-with-break-continue" tabindex="-1">Loop with break/continue <a class="header-anchor" href="#loop-with-break-continue" aria-label="Permalink to &quot;Loop with break/continue&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>for i &lt; 10 {</span></span>
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
<span class="line"><span>      Print:</span></span>
<span class="line"><span>        Identifier: i</span></span></code></pre></div>`,22)])])}const g=n(i,[["render",l]]);export{u as __pageData,g as default};
