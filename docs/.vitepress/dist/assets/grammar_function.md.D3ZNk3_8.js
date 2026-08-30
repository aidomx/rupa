import{_ as s,o as a,c as e,a2 as p}from"./chunks/framework.BXzK3EA4.js";const d=JSON.parse('{"title":"Function Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/function.md","filePath":"grammar/function.md"}'),l={name:"grammar/function.md"};function r(i,n,c,t,o,b){return a(),e("div",null,[...n[0]||(n[0]=[p(`<h1 id="function-grammar" tabindex="-1">Function Grammar <a class="header-anchor" href="#function-grammar" aria-label="Permalink to &quot;Function Grammar&quot;">​</a></h1><p>Function declaration dan function call menggunakan pola awal yang sama: identifier diikuti parentheses. Parser membedakan declaration ketika bentuk tersebut diikuti body block.</p><h2 id="declaration" tabindex="-1">Declaration <a class="header-anchor" href="#declaration" aria-label="Permalink to &quot;Declaration&quot;">​</a></h2><p>Source:</p><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#B392F0;">add</span><span style="color:#E1E4E8;">(x: number, y: number) {</span></span>
<span class="line"><span style="color:#F97583;">    return</span><span style="color:#E1E4E8;"> x </span><span style="color:#F97583;">+</span><span style="color:#E1E4E8;"> y</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
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
<span class="line"><span>            Identifier: y</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br><span class="line-number">10</span><br><span class="line-number">11</span><br><span class="line-number">12</span><br><span class="line-number">13</span><br><span class="line-number">14</span><br><span class="line-number">15</span><br><span class="line-number">16</span><br><span class="line-number">17</span><br></div></div><h2 id="call-statement" tabindex="-1">Call statement <a class="header-anchor" href="#call-statement" aria-label="Permalink to &quot;Call statement&quot;">​</a></h2><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#B392F0;">add</span><span style="color:#E1E4E8;">(</span><span style="color:#79B8FF;">1</span><span style="color:#E1E4E8;">, </span><span style="color:#79B8FF;">2</span><span style="color:#E1E4E8;">)</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br></div></div><p>Jika pola identifier + arguments tidak membentuk declaration dengan body, parser membuat <code>Call</code>:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Call:</span></span>
<span class="line"><span>    Callee:</span></span>
<span class="line"><span>      Identifier: add</span></span>
<span class="line"><span>    Arg 1:</span></span>
<span class="line"><span>      Number: 1</span></span>
<span class="line"><span>    Arg 2:</span></span>
<span class="line"><span>      Number: 2</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br></div></div><h2 id="function-with-object-return" tabindex="-1">Function with object return <a class="header-anchor" href="#function-with-object-return" aria-label="Permalink to &quot;Function with object return&quot;">​</a></h2><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#B392F0;">makePoint</span><span style="color:#E1E4E8;">(px, py) {</span></span>
<span class="line"><span style="color:#F97583;">    return</span><span style="color:#E1E4E8;"> { x: px, y: py }</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
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
<span class="line"><span>              Value: Identifier: py</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br><span class="line-number">10</span><br><span class="line-number">11</span><br><span class="line-number">12</span><br><span class="line-number">13</span><br><span class="line-number">14</span><br><span class="line-number">15</span><br><span class="line-number">16</span><br><span class="line-number">17</span><br></div></div>`,15)])])}const m=s(l,[["render",r]]);export{d as __pageData,m as default};
