import{_ as n,o as a,c as e,a2 as p}from"./chunks/framework.BXzK3EA4.js";const m=JSON.parse('{"title":"Struct Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/struct.md","filePath":"grammar/struct.md"}'),l={name:"grammar/struct.md"};function r(i,s,c,t,o,b){return a(),e("div",null,[...s[0]||(s[0]=[p(`<h1 id="struct-grammar" tabindex="-1">Struct Grammar <a class="header-anchor" href="#struct-grammar" aria-label="Permalink to &quot;Struct Grammar&quot;">​</a></h1><p>Grammar struct membentuk node struct dari nama dan field declarations.</p><h2 id="simple-struct" tabindex="-1">Simple struct <a class="header-anchor" href="#simple-struct" aria-label="Permalink to &quot;Simple struct&quot;">​</a></h2><p>Source:</p><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#E1E4E8;">People {</span></span>
<span class="line"><span style="color:#B392F0;">    name</span><span style="color:#E1E4E8;">: string</span></span>
<span class="line"><span style="color:#B392F0;">    age</span><span style="color:#E1E4E8;">: number</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Struct:</span></span>
<span class="line"><span>    Name: Identifier: People</span></span>
<span class="line"><span>    Body:</span></span>
<span class="line"><span>      Block:</span></span>
<span class="line"><span>        Annotation:</span></span>
<span class="line"><span>          Name: Identifier: name</span></span>
<span class="line"><span>          Type: Identifier: string</span></span>
<span class="line"><span>        Annotation:</span></span>
<span class="line"><span>          Name: Identifier: age</span></span>
<span class="line"><span>          Type: Identifier: number</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br><span class="line-number">10</span><br><span class="line-number">11</span><br></div></div><h2 id="struct-usage" tabindex="-1">Struct usage <a class="header-anchor" href="#struct-usage" aria-label="Permalink to &quot;Struct usage&quot;">​</a></h2><p>Struct berfungsi sebagai blueprint untuk membuat object:</p><div class="language-js line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span style="color:#E1E4E8;">People {</span></span>
<span class="line"><span style="color:#B392F0;">    name</span><span style="color:#E1E4E8;">: string</span></span>
<span class="line"><span style="color:#B392F0;">    age</span><span style="color:#E1E4E8;">: number</span></span>
<span class="line"><span style="color:#E1E4E8;">}</span></span>
<span class="line"></span>
<span class="line"><span style="color:#E1E4E8;">person </span><span style="color:#F97583;">=</span><span style="color:#E1E4E8;"> { name: </span><span style="color:#9ECBFF;">&quot;Rupa&quot;</span><span style="color:#E1E4E8;">, age: </span><span style="color:#79B8FF;">20</span><span style="color:#E1E4E8;"> }</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br></div></div><p>AST:</p><div class="language-text line-numbers-mode"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
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
<span class="line"><span>          Value: Number: 20</span></span></code></pre><div class="line-numbers-wrapper" aria-hidden="true"><span class="line-number">1</span><br><span class="line-number">2</span><br><span class="line-number">3</span><br><span class="line-number">4</span><br><span class="line-number">5</span><br><span class="line-number">6</span><br><span class="line-number">7</span><br><span class="line-number">8</span><br><span class="line-number">9</span><br><span class="line-number">10</span><br><span class="line-number">11</span><br><span class="line-number">12</span><br><span class="line-number">13</span><br><span class="line-number">14</span><br><span class="line-number">15</span><br><span class="line-number">16</span><br><span class="line-number">17</span><br><span class="line-number">18</span><br><span class="line-number">19</span><br><span class="line-number">20</span><br><span class="line-number">21</span><br></div></div>`,12)])])}const d=n(l,[["render",r]]);export{m as __pageData,d as default};
