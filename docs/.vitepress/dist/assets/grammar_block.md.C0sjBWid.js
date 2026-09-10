import{_ as s,o as n,c as p,a0 as e}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Block Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/block.md","filePath":"grammar/block.md"}'),l={name:"grammar/block.md"};function i(t,a,c,o,r,d){return n(),p("div",null,[...a[0]||(a[0]=[e(`<h1 id="block-grammar" tabindex="-1">Block Grammar <a class="header-anchor" href="#block-grammar" aria-label="Permalink to &quot;Block Grammar&quot;">​</a></h1><p>Grammar block membentuk node block dari statements.</p><h2 id="simple-block" tabindex="-1">Simple block <a class="header-anchor" href="#simple-block" aria-label="Permalink to &quot;Simple block&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if true {</span></span>
<span class="line"><span>    print(&quot;inside block&quot;)</span></span>
<span class="line"><span>    print(&quot;still inside&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  If:</span></span>
<span class="line"><span>    Condition: Boolean: true</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Print: String: inside block</span></span>
<span class="line"><span>      Print: String: still inside</span></span></code></pre></div><h2 id="function-body" tabindex="-1">Function body <a class="header-anchor" href="#function-body" aria-label="Permalink to &quot;Function body&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x, y) {</span></span>
<span class="line"><span>    result = x + y</span></span>
<span class="line"><span>    return result</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Function:</span></span>
<span class="line"><span>    Name: Identifier: add</span></span>
<span class="line"><span>    Parameters:</span></span>
<span class="line"><span>      Identifier: x</span></span>
<span class="line"><span>      Identifier: y</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      Assignment:</span></span>
<span class="line"><span>        Target: Identifier: result</span></span>
<span class="line"><span>        Value: Binary: +</span></span>
<span class="line"><span>          Left: Identifier: x</span></span>
<span class="line"><span>          Right: Identifier: y</span></span>
<span class="line"><span>      Return: Identifier: result</span></span></code></pre></div><h2 id="nested-blocks" tabindex="-1">Nested blocks <a class="header-anchor" href="#nested-blocks" aria-label="Permalink to &quot;Nested blocks&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if x &gt; 0 {</span></span>
<span class="line"><span>    if x &gt; 10 {</span></span>
<span class="line"><span>        print(&quot;large&quot;)</span></span>
<span class="line"><span>    } else {</span></span>
<span class="line"><span>        print(&quot;small&quot;)</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  If:</span></span>
<span class="line"><span>    Condition: Binary: &gt;</span></span>
<span class="line"><span>      Left: Identifier: x</span></span>
<span class="line"><span>      Right: Number: 0</span></span>
<span class="line"><span>    Body: Block</span></span>
<span class="line"><span>      If:</span></span>
<span class="line"><span>        Condition: Binary: &gt;</span></span>
<span class="line"><span>          Left: Identifier: x</span></span>
<span class="line"><span>          Right: Number: 10</span></span>
<span class="line"><span>        Body: Block</span></span>
<span class="line"><span>          Print: String: large</span></span>
<span class="line"><span>        Else: Block</span></span>
<span class="line"><span>          Print: String: small</span></span></code></pre></div>`,17)])])}const b=s(l,[["render",i]]);export{h as __pageData,b as default};
