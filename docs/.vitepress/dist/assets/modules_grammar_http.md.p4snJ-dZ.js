import{_ as s,o as n,c as e,a0 as p}from"./chunks/framework.BpJS36ta.js";const u=JSON.parse('{"title":"HTTP Module Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"modules/grammar/http.md","filePath":"modules/grammar/http.md"}'),t={name:"modules/grammar/http.md"};function l(i,a,r,o,c,d){return n(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="http-module-grammar" tabindex="-1">HTTP Module Grammar <a class="header-anchor" href="#http-module-grammar" aria-label="Permalink to &quot;HTTP Module Grammar&quot;">​</a></h1><p>Module <code>http</code> diimpor dari root namespace Rupa:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span></code></pre></div><h2 id="ast-structure" tabindex="-1">AST Structure <a class="header-anchor" href="#ast-structure" aria-label="Permalink to &quot;AST Structure&quot;">​</a></h2><h3 id="http-server-port-handler" tabindex="-1"><code>http.server(port, handler?)</code> <a class="header-anchor" href="#http-server-port-handler" aria-label="Permalink to &quot;\`http.server(port, handler?)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: http</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: server</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Number: 8080</span></span></code></pre></div><h3 id="http-stop-handle" tabindex="-1"><code>http.stop(handle)</code> <a class="header-anchor" href="#http-stop-handle" aria-label="Permalink to &quot;\`http.stop(handle)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: http</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: stop</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Identifier: server</span></span></code></pre></div><h3 id="http-request-method-url-data" tabindex="-1"><code>http.request(method, url, data?)</code> <a class="header-anchor" href="#http-request-method-url-data" aria-label="Permalink to &quot;\`http.request(method, url, data?)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: http</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: request</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    String: &quot;GET&quot;</span></span>
<span class="line"><span>  Arg 2:</span></span>
<span class="line"><span>    String: &quot;https://api.example.com/data&quot;</span></span></code></pre></div><h3 id="http-get-url" tabindex="-1"><code>http.get(url)</code> <a class="header-anchor" href="#http-get-url" aria-label="Permalink to &quot;\`http.get(url)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: http</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: get</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    String: &quot;https://api.example.com/data&quot;</span></span></code></pre></div><h3 id="http-post-url-data" tabindex="-1"><code>http.post(url, data)</code> <a class="header-anchor" href="#http-post-url-data" aria-label="Permalink to &quot;\`http.post(url, data)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: http</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: post</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    String: &quot;https://api.example.com/data&quot;</span></span>
<span class="line"><span>  Arg 2:</span></span>
<span class="line"><span>    String: &quot;name=Rupa&quot;</span></span></code></pre></div><h2 id="module-structure" tabindex="-1">Module Structure <a class="header-anchor" href="#module-structure" aria-label="Permalink to &quot;Module Structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>http</span></span>
<span class="line"><span>├── server(port, handler?) — start HTTP server</span></span>
<span class="line"><span>├── stop(handle)           — stop server</span></span>
<span class="line"><span>├── request(method, url, data?) — HTTP client</span></span>
<span class="line"><span>├── get(url)               — convenience GET</span></span>
<span class="line"><span>├── post(url, data)        — convenience POST</span></span>
<span class="line"><span>├── put(url, data)         — convenience PUT</span></span>
<span class="line"><span>├── delete(url)            — convenience DELETE</span></span>
<span class="line"><span>└── patch(url, data)       — convenience PATCH</span></span></code></pre></div><h2 id="implementation-notes" tabindex="-1">Implementation Notes <a class="header-anchor" href="#implementation-notes" aria-label="Permalink to &quot;Implementation Notes&quot;">​</a></h2><ul><li>Server uses POSIX sockets (socket, bind, listen, accept)</li><li>Client uses curl (must be installed)</li><li>Server runs in background thread</li><li>Handler called on main thread (thread-safe queue design)</li><li>Request object: req.method, req.path, req.headers, req.body</li><li>Response object: res.status, res.body, res.setHeader(), res.json()</li></ul>`,18)])])}const m=s(t,[["render",l]]);export{u as __pageData,m as default};
