import{_ as a,o as n,c as i,a0 as e}from"./chunks/framework.CShDTZbC.js";const o=JSON.parse('{"title":"Case Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/case.md","filePath":"grammar/case.md"}'),p={name:"grammar/case.md"};function t(l,s,r,h,c,d){return n(),i("div",null,[...s[0]||(s[0]=[e(`<h1 id="case-grammar" tabindex="-1">Case Grammar <a class="header-anchor" href="#case-grammar" aria-label="Permalink to &quot;Case Grammar&quot;">​</a></h1><p>Grammar case membentuk node case dari subject dan entries.</p><h2 id="simple-case" tabindex="-1">Simple case <a class="header-anchor" href="#simple-case" aria-label="Permalink to &quot;Simple case&quot;">​</a></h2><p>Source:</p><div class="language-js vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">case</span><span style="--shiki-light:#E36209;--shiki-dark:#FFAB70;"> status</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;"> =&gt;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">    200</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">: </span><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">print</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">&quot;success&quot;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">)</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">    404</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">: </span><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">print</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">&quot;not found&quot;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">)</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    default</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">: </span><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">print</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">&quot;unknown&quot;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">)</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
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
<span class="line"><span>    └── Body: Print</span></span></code></pre></div><p>Wildcard <code>default</code> menangkap semua case yang tidak terpenuhi.</p>`,10)])])}const u=a(p,[["render",t]]);export{o as __pageData,u as default};
