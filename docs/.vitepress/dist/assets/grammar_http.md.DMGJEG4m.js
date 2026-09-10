import{_ as s,o as n,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const u=JSON.parse('{"title":"HTTP Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/http.md","filePath":"grammar/http.md"}'),t={name:"grammar/http.md"};function l(i,a,r,o,c,d){return n(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="http-grammar" tabindex="-1">HTTP Grammar <a class="header-anchor" href="#http-grammar" aria-label="Permalink to &quot;HTTP Grammar&quot;">​</a></h1><p>Grammar HTTP module mendefinisikan struktur AST untuk HTTP server dan client.</p><h2 id="module-structure" tabindex="-1">Module Structure <a class="header-anchor" href="#module-structure" aria-label="Permalink to &quot;Module Structure&quot;">​</a></h2><p>HTTP module adalah C stdlib yang diakses melalui <code>import http from rupa</code>.</p><h2 id="http-server" tabindex="-1">HTTP Server <a class="header-anchor" href="#http-server" aria-label="Permalink to &quot;HTTP Server&quot;">​</a></h2><h3 id="http-server-port" tabindex="-1"><code>http.server(port)</code> <a class="header-anchor" href="#http-server-port" aria-label="Permalink to &quot;\`http.server(port)\`&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>server = http.server(8080)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Assignment:</span></span>
<span class="line"><span>  Target:</span></span>
<span class="line"><span>    Identifier: server</span></span>
<span class="line"><span>  Value:</span></span>
<span class="line"><span>    Call:</span></span>
<span class="line"><span>      Callee:</span></span>
<span class="line"><span>        Member:</span></span>
<span class="line"><span>          Object:</span></span>
<span class="line"><span>            Identifier: http</span></span>
<span class="line"><span>          Member:</span></span>
<span class="line"><span>            Identifier: server</span></span>
<span class="line"><span>      Arg 1:</span></span>
<span class="line"><span>        Number: 8080</span></span></code></pre></div><h3 id="http-stop-handle" tabindex="-1"><code>http.stop(handle)</code> <a class="header-anchor" href="#http-stop-handle" aria-label="Permalink to &quot;\`http.stop(handle)\`&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>http.stop(server)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: http</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: stop</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Identifier: server</span></span></code></pre></div><h2 id="http-client" tabindex="-1">HTTP Client <a class="header-anchor" href="#http-client" aria-label="Permalink to &quot;HTTP Client&quot;">​</a></h2><h3 id="http-request-method-url-data" tabindex="-1"><code>http.request(method, url, data?)</code> <a class="header-anchor" href="#http-request-method-url-data" aria-label="Permalink to &quot;\`http.request(method, url, data?)\`&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>response = http.request(&quot;GET&quot;, &quot;https://api.example.com/data&quot;)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Assignment:</span></span>
<span class="line"><span>  Target:</span></span>
<span class="line"><span>    Identifier: response</span></span>
<span class="line"><span>  Value:</span></span>
<span class="line"><span>    Call:</span></span>
<span class="line"><span>      Callee:</span></span>
<span class="line"><span>        Member:</span></span>
<span class="line"><span>          Object:</span></span>
<span class="line"><span>            Identifier: http</span></span>
<span class="line"><span>          Member:</span></span>
<span class="line"><span>            Identifier: request</span></span>
<span class="line"><span>      Arg 1:</span></span>
<span class="line"><span>        String: &quot;GET&quot;</span></span>
<span class="line"><span>      Arg 2:</span></span>
<span class="line"><span>        String: &quot;https://api.example.com/data&quot;</span></span></code></pre></div><h3 id="http-get-url" tabindex="-1"><code>http.get(url)</code> <a class="header-anchor" href="#http-get-url" aria-label="Permalink to &quot;\`http.get(url)\`&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>response = http.get(&quot;https://api.example.com/data&quot;)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Assignment:</span></span>
<span class="line"><span>  Target:</span></span>
<span class="line"><span>    Identifier: response</span></span>
<span class="line"><span>  Value:</span></span>
<span class="line"><span>    Call:</span></span>
<span class="line"><span>      Callee:</span></span>
<span class="line"><span>        Member:</span></span>
<span class="line"><span>          Object:</span></span>
<span class="line"><span>            Identifier: http</span></span>
<span class="line"><span>          Member:</span></span>
<span class="line"><span>            Identifier: get</span></span>
<span class="line"><span>      Arg 1:</span></span>
<span class="line"><span>        String: &quot;https://api.example.com/data&quot;</span></span></code></pre></div><h3 id="http-post-url-data" tabindex="-1"><code>http.post(url, data)</code> <a class="header-anchor" href="#http-post-url-data" aria-label="Permalink to &quot;\`http.post(url, data)\`&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>response = http.post(&quot;https://api.example.com/data&quot;, &quot;name=Rupa&quot;)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Assignment:</span></span>
<span class="line"><span>  Target:</span></span>
<span class="line"><span>    Identifier: response</span></span>
<span class="line"><span>  Value:</span></span>
<span class="line"><span>    Call:</span></span>
<span class="line"><span>      Callee:</span></span>
<span class="line"><span>        Member:</span></span>
<span class="line"><span>          Object:</span></span>
<span class="line"><span>            Identifier: http</span></span>
<span class="line"><span>          Member:</span></span>
<span class="line"><span>            Identifier: post</span></span>
<span class="line"><span>      Arg 1:</span></span>
<span class="line"><span>        String: &quot;https://api.example.com/data&quot;</span></span>
<span class="line"><span>      Arg 2:</span></span>
<span class="line"><span>        String: &quot;name=Rupa&quot;</span></span></code></pre></div><h2 id="function-import" tabindex="-1">Function Import <a class="header-anchor" href="#function-import" aria-label="Permalink to &quot;Function Import&quot;">​</a></h2><p>Fungsi bisa diimport langsung:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import server, stop, get, post from rupa.http</span></span>
<span class="line"><span></span></span>
<span class="line"><span>s = server(8080)</span></span>
<span class="line"><span>stop(s)</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Module statement:</span></span>
<span class="line"><span>  ArrayLiteral:</span></span>
<span class="line"><span>    Literal ID: server</span></span>
<span class="line"><span>    Literal ID: stop</span></span>
<span class="line"><span>    Literal ID: get</span></span>
<span class="line"><span>    Literal ID: post</span></span>
<span class="line"><span>  from:</span></span>
<span class="line"><span>    Literal ID: rupa.http</span></span></code></pre></div><h2 id="http-structure" tabindex="-1">HTTP Structure <a class="header-anchor" href="#http-structure" aria-label="Permalink to &quot;HTTP Structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>http</span></span>
<span class="line"><span>├── server(port) -&gt; handle</span></span>
<span class="line"><span>├── stop(handle)</span></span>
<span class="line"><span>├── request(method, url, data?) -&gt; response</span></span>
<span class="line"><span>├── get(url) -&gt; response</span></span>
<span class="line"><span>├── post(url, data) -&gt; response</span></span>
<span class="line"><span>├── put(url, data) -&gt; response</span></span>
<span class="line"><span>├── delete(url) -&gt; response</span></span>
<span class="line"><span>└── patch(url, data) -&gt; response</span></span></code></pre></div><h2 id="status-codes" tabindex="-1">Status Codes <a class="header-anchor" href="#status-codes" aria-label="Permalink to &quot;Status Codes&quot;">​</a></h2><p>Response mengembalikan string body. Status code belum tersedia di versi ini.</p><h2 id="implementation-details" tabindex="-1">Implementation Details <a class="header-anchor" href="#implementation-details" aria-label="Permalink to &quot;Implementation Details&quot;">​</a></h2><ul><li>Server menggunakan POSIX sockets (<code>socket</code>, <code>bind</code>, <code>listen</code>, <code>accept</code>)</li><li>Client menggunakan <code>curl</code> atau <code>wget</code> (harus terinstall)</li><li>Server berjalan di background thread</li><li>Timeout default: 30 detik</li></ul>`,37)])])}const g=s(t,[["render",l]]);export{u as __pageData,g as default};
