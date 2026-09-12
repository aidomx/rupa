import{_ as n,o as s,c as e,a0 as t}from"./chunks/framework.BpJS36ta.js";const h=JSON.parse('{"title":"Net Module Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"modules/grammar/net.md","filePath":"modules/grammar/net.md"}'),p={name:"modules/grammar/net.md"};function l(c,a,r,i,o,d){return s(),e("div",null,[...a[0]||(a[0]=[t(`<h1 id="net-module-grammar" tabindex="-1">Net Module Grammar <a class="header-anchor" href="#net-module-grammar" aria-label="Permalink to &quot;Net Module Grammar&quot;">​</a></h1><h2 id="ast-structure" tabindex="-1">AST Structure <a class="header-anchor" href="#ast-structure" aria-label="Permalink to &quot;AST Structure&quot;">​</a></h2><h3 id="net-connect-host-port" tabindex="-1"><code>net.connect(host, port)</code> <a class="header-anchor" href="#net-connect-host-port" aria-label="Permalink to &quot;\`net.connect(host, port)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: net</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: connect</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    String: &quot;127.0.0.1&quot;</span></span>
<span class="line"><span>  Arg 2:</span></span>
<span class="line"><span>    Number: 8080</span></span></code></pre></div><h3 id="net-accept-fd" tabindex="-1"><code>net.accept(fd)</code> <a class="header-anchor" href="#net-accept-fd" aria-label="Permalink to &quot;\`net.accept(fd)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: net</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: accept</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Identifier: server</span></span></code></pre></div><h2 id="module-structure" tabindex="-1">Module Structure <a class="header-anchor" href="#module-structure" aria-label="Permalink to &quot;Module Structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>net</span></span>
<span class="line"><span>├── connect(host, port)   — TCP connect</span></span>
<span class="line"><span>├── send(fd, data)        — send data</span></span>
<span class="line"><span>├── receive(fd, size?)    — receive data</span></span>
<span class="line"><span>├── close(fd)             — close socket</span></span>
<span class="line"><span>├── listen(port, backlog?) — TCP server</span></span>
<span class="line"><span>├── accept(fd)            — accept connection</span></span>
<span class="line"><span>└── resolve(host)         — DNS resolve</span></span></code></pre></div>`,8)])])}const m=n(p,[["render",l]]);export{h as __pageData,m as default};
