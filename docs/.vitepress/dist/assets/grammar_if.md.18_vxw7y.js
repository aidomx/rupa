import{_ as s,o as n,c as p,a0 as e}from"./chunks/framework.CShDTZbC.js";const u=JSON.parse('{"title":"If Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/if.md","filePath":"grammar/if.md"}'),i={name:"grammar/if.md"};function t(l,a,o,c,r,d){return n(),p("div",null,[...a[0]||(a[0]=[e(`<h1 id="if-grammar" tabindex="-1">If Grammar <a class="header-anchor" href="#if-grammar" aria-label="Permalink to &quot;If Grammar&quot;">​</a></h1><p>Grammar if membentuk node if dari condition, body, dan branch opsional.</p><h2 id="simple-if" tabindex="-1">Simple if <a class="header-anchor" href="#simple-if" aria-label="Permalink to &quot;Simple if&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if x &gt; 0 {</span></span>
<span class="line"><span>    print(&quot;positive&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  If:</span></span>
<span class="line"><span>    Condition: Binary: &gt;</span></span>
<span class="line"><span>      Left: Identifier: x</span></span>
<span class="line"><span>      Right: Number: 0</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print: String: positive</span></span></code></pre></div><h2 id="if-else" tabindex="-1">If-else <a class="header-anchor" href="#if-else" aria-label="Permalink to &quot;If-else&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if x &gt; 0 {</span></span>
<span class="line"><span>    print(&quot;positive&quot;)</span></span>
<span class="line"><span>} else {</span></span>
<span class="line"><span>    print(&quot;non-positive&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  If:</span></span>
<span class="line"><span>    Condition: Binary: &gt;</span></span>
<span class="line"><span>      Left: Identifier: x</span></span>
<span class="line"><span>      Right: Number: 0</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print: String: positive</span></span>
<span class="line"><span>    Else: Block</span></span>
<span class="line"><span>      Print: String: non-positive</span></span></code></pre></div><h2 id="if-elseif-else" tabindex="-1">If-elseif-else <a class="header-anchor" href="#if-elseif-else" aria-label="Permalink to &quot;If-elseif-else&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if x &gt; 0 {</span></span>
<span class="line"><span>    print(&quot;positive&quot;)</span></span>
<span class="line"><span>} elseif x == 0 {</span></span>
<span class="line"><span>    print(&quot;zero&quot;)</span></span>
<span class="line"><span>} else {</span></span>
<span class="line"><span>    print(&quot;negative&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  If:</span></span>
<span class="line"><span>    Condition: Binary: &gt;</span></span>
<span class="line"><span>      Left: Identifier: x</span></span>
<span class="line"><span>      Right: Number: 0</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print: String: positive</span></span>
<span class="line"><span>    Else: If</span></span>
<span class="line"><span>      Condition: Binary: ==</span></span>
<span class="line"><span>        Left: Identifier: x</span></span>
<span class="line"><span>        Right: Number: 0</span></span>
<span class="line"><span>      Body: Block</span></span>
<span class="line"><span>        Print: String: zero</span></span>
<span class="line"><span>      Else: Block</span></span>
<span class="line"><span>        Print: String: negative</span></span></code></pre></div><h2 id="inline-if" tabindex="-1">Inline if <a class="header-anchor" href="#inline-if" aria-label="Permalink to &quot;Inline if&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if x &gt; 0: print(true)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  If:</span></span>
<span class="line"><span>    Condition: Binary: &gt;</span></span>
<span class="line"><span>      Left: Identifier: x</span></span>
<span class="line"><span>      Right: Number: 0</span></span>
<span class="line"><span>    Body: Print: Boolean: true</span></span></code></pre></div>`,22)])])}const g=s(i,[["render",t]]);export{u as __pageData,g as default};
