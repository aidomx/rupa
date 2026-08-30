import{_ as n,o as a,c as p,a2 as e}from"./chunks/framework.BXzK3EA4.js";const u=JSON.parse('{"title":"Loop Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/loop.md","filePath":"grammar/loop.md"}'),l={name:"grammar/loop.md"};function r(i,s,o,c,t,b){return a(),p("div",null,[...s[0]||(s[0]=[e(`<h1 id="loop-grammar" tabindex="-1">Loop Grammar <a class="header-anchor" href="#loop-grammar" aria-label="Permalink to &quot;Loop Grammar&quot;">​</a></h1><p>Grammar loop membentuk node loop dari kind, condition, dan body.</p><h2 id="for-loop" tabindex="-1">For loop <a class="header-anchor" href="#for-loop" aria-label="Permalink to &quot;For loop&quot;">​</a></h2><p>Source:</p><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#E1E4E8;">for i </span><span style="color:#F97583;">&lt;</span><span style="color:#79B8FF;"> 10</span><span style="color:#E1E4E8;"> {</span></span>
<span class="line"><span style="color:#B392F0;">    print</span><span style="color:#E1E4E8;">(i)</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Loop: for</span></span>
<span class="line"><span>    Condition: Binary: &lt;</span></span>
<span class="line"><span>      Left: Identifier: i</span></span>
<span class="line"><span>      Right: Number: 10</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print:</span></span>
<span class="line"><span>        Identifier: i</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br></div></div><h2 id="rev-loop" tabindex="-1">Rev loop <a class="header-anchor" href="#rev-loop" aria-label="Permalink to &quot;Rev loop&quot;">​</a></h2><p>Source:</p><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#E1E4E8;">rev i </span><span style="color:#F97583;">&gt;</span><span style="color:#79B8FF;"> 0</span><span style="color:#E1E4E8;"> {</span></span>
<span class="line"><span style="color:#B392F0;">    print</span><span style="color:#E1E4E8;">(i)</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Loop: rev</span></span>
<span class="line"><span>    Condition: Binary: &gt;</span></span>
<span class="line"><span>      Left: Identifier: i</span></span>
<span class="line"><span>      Right: Number: 0</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print:</span></span>
<span class="line"><span>        Identifier: i</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br></div></div><h2 id="while-loop" tabindex="-1">While loop <a class="header-anchor" href="#while-loop" aria-label="Permalink to &quot;While loop&quot;">​</a></h2><p>Source:</p><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#F97583;">while</span><span style="color:#E1E4E8;"> x </span><span style="color:#F97583;">&lt;</span><span style="color:#79B8FF;"> 10</span><span style="color:#E1E4E8;"> {</span></span>
<span class="line"><span style="color:#B392F0;">    print</span><span style="color:#E1E4E8;">(x)</span></span>
<span class="line"><span style="color:#E1E4E8;">    x</span><span style="color:#F97583;">++</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Loop: while</span></span>
<span class="line"><span>    Condition: Binary: &lt;</span></span>
<span class="line"><span>      Left: Identifier: x</span></span>
<span class="line"><span>      Right: Number: 10</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print:</span></span>
<span class="line"><span>        Identifier: x</span></span>
<span class="line"><span>      Update: postfix ++</span></span>
<span class="line"><span>        Identifier: x</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br><span class="line-number">10</span><br></div></div><h2 id="loop-with-break-continue" tabindex="-1">Loop with break/continue <a class="header-anchor" href="#loop-with-break-continue" aria-label="Permalink to &quot;Loop with break/continue&quot;">​</a></h2><p>Source:</p><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#E1E4E8;">for i </span><span style="color:#F97583;">&lt;</span><span style="color:#79B8FF;"> 10</span><span style="color:#E1E4E8;"> {</span></span>
<span class="line"><span style="color:#F97583;">    if</span><span style="color:#E1E4E8;"> i </span><span style="color:#F97583;">==</span><span style="color:#79B8FF;"> 3</span><span style="color:#E1E4E8;">: </span><span style="color:#F97583;">continue</span></span>
<span class="line"><span style="color:#F97583;">    if</span><span style="color:#E1E4E8;"> i </span><span style="color:#F97583;">==</span><span style="color:#79B8FF;"> 7</span><span style="color:#E1E4E8;">: </span><span style="color:#F97583;">break</span></span>
<span class="line"><span style="color:#B392F0;">    print</span><span style="color:#E1E4E8;">(i)</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
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
<span class="line"><span>        Identifier: i</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br><span class="line-number">10</span><br><span class="line-number">11</span><br><span class="line-number">12</span><br><span class="line-number">13</span><br><span class="line-number">14</span><br><span class="line-number">15</span><br><span class="line-number">16</span><br><span class="line-number">17</span><br><span class="line-number">18</span><br></div></div>`,22)])])}const m=n(l,[["render",r]]);export{u as __pageData,m as default};
