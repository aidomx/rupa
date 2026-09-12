import{_ as s,o as n,c as e,a0 as t}from"./chunks/framework.BpJS36ta.js";const u=JSON.parse('{"title":"DateTime Module Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"modules/grammar/datetime.md","filePath":"modules/grammar/datetime.md"}'),p={name:"modules/grammar/datetime.md"};function i(l,a,r,c,o,d){return n(),e("div",null,[...a[0]||(a[0]=[t(`<h1 id="datetime-module-grammar" tabindex="-1">DateTime Module Grammar <a class="header-anchor" href="#datetime-module-grammar" aria-label="Permalink to &quot;DateTime Module Grammar&quot;">​</a></h1><h2 id="ast-structure" tabindex="-1">AST Structure <a class="header-anchor" href="#ast-structure" aria-label="Permalink to &quot;AST Structure&quot;">​</a></h2><h3 id="datetime-now" tabindex="-1"><code>datetime.now()</code> <a class="header-anchor" href="#datetime-now" aria-label="Permalink to &quot;\`datetime.now()\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: datetime</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: now</span></span></code></pre></div><h3 id="datetime-format-ts-fmt" tabindex="-1"><code>datetime.format(ts, fmt)</code> <a class="header-anchor" href="#datetime-format-ts-fmt" aria-label="Permalink to &quot;\`datetime.format(ts, fmt)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: datetime</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: format</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Identifier: ts</span></span>
<span class="line"><span>  Arg 2:</span></span>
<span class="line"><span>    String: &quot;%Y-%m-%d&quot;</span></span></code></pre></div><h2 id="module-structure" tabindex="-1">Module Structure <a class="header-anchor" href="#module-structure" aria-label="Permalink to &quot;Module Structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>datetime</span></span>
<span class="line"><span>├── now()          — current Unix timestamp</span></span>
<span class="line"><span>├── nowMs()        — current timestamp in ms</span></span>
<span class="line"><span>├── format(ts, fmt?) — format timestamp to string</span></span>
<span class="line"><span>├── parse(str, fmt?) — parse string to timestamp</span></span>
<span class="line"><span>├── diff(ts1, ts2) — difference in seconds</span></span>
<span class="line"><span>├── add(ts, secs)  — add seconds to timestamp</span></span>
<span class="line"><span>├── year(ts?)      — extract year</span></span>
<span class="line"><span>├── month(ts?)     — extract month</span></span>
<span class="line"><span>├── day(ts?)       — extract day</span></span>
<span class="line"><span>├── hour(ts?)      — extract hour</span></span>
<span class="line"><span>├── minute(ts?)    — extract minute</span></span>
<span class="line"><span>└── second(ts?)    — extract second</span></span></code></pre></div>`,8)])])}const h=s(p,[["render",i]]);export{u as __pageData,h as default};
