import{_ as s,o as n,c as e,a0 as p}from"./chunks/framework.BpJS36ta.js";const m=JSON.parse('{"title":"Case Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/case.md","filePath":"grammar/case.md"}'),t={name:"grammar/case.md"};function i(l,a,r,c,o,d){return n(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="case-grammar" tabindex="-1">Case Grammar <a class="header-anchor" href="#case-grammar" aria-label="Permalink to &quot;Case Grammar&quot;">​</a></h1><p>Grammar case membentuk node case dari subject dan entries.</p><h2 id="simple-case" tabindex="-1">Simple case <a class="header-anchor" href="#simple-case" aria-label="Permalink to &quot;Simple case&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>case status =&gt; {</span></span>
<span class="line"><span>    200: print(&quot;success&quot;)</span></span>
<span class="line"><span>    404: print(&quot;not found&quot;)</span></span>
<span class="line"><span>    default: print(&quot;unknown&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Case:</span></span>
<span class="line"><span>    Subject: Identifier: status</span></span>
<span class="line"><span>    Entry:</span></span>
<span class="line"><span>      Pattern: Number: 200</span></span>
<span class="line"><span>      Body: Print: String: success</span></span>
<span class="line"><span>    Entry:</span></span>
<span class="line"><span>      Pattern: Number: 404</span></span>
<span class="line"><span>      Body: Print: String: not found</span></span>
<span class="line"><span>    Wildcard:</span></span>
<span class="line"><span>      Body: Print: String: unknown</span></span></code></pre></div><h2 id="case-structure" tabindex="-1">Case structure <a class="header-anchor" href="#case-structure" aria-label="Permalink to &quot;Case structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Case</span></span>
<span class="line"><span>├── Subject</span></span>
<span class="line"><span>│   └── Identifier: status</span></span>
<span class="line"><span>├── Entry</span></span>
<span class="line"><span>│   ├── Pattern: Number: 200</span></span>
<span class="line"><span>│   └── Body: Print</span></span>
<span class="line"><span>├── Entry</span></span>
<span class="line"><span>│   ├── Pattern: Number: 404</span></span>
<span class="line"><span>│   └── Body: Print</span></span>
<span class="line"><span>└── Wildcard</span></span>
<span class="line"><span>    └── Body: Print</span></span></code></pre></div><p>Wildcard <code>default</code> menangkap semua case yang tidak terpenuhi.</p>`,10)])])}const h=s(t,[["render",i]]);export{m as __pageData,h as default};
