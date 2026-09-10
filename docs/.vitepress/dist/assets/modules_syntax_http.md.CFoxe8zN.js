import{_ as s,o as e,c as n,a0 as p}from"./chunks/framework.BpJS36ta.js";const u=JSON.parse('{"title":"HTTP Module","description":"","frontmatter":{},"headers":[],"relativePath":"modules/syntax/http.md","filePath":"modules/syntax/http.md"}'),t={name:"modules/syntax/http.md"};function o(l,a,r,i,d,h){return e(),n("div",null,[...a[0]||(a[0]=[p(`<h1 id="http-module" tabindex="-1">HTTP Module <a class="header-anchor" href="#http-module" aria-label="Permalink to &quot;HTTP Module&quot;">​</a></h1><p>Modul HTTP menyediakan client dan server HTTP sederhana.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span></code></pre></div><h2 id="http-server-port-handler" tabindex="-1"><code>http.server(port, handler?)</code> <a class="header-anchor" href="#http-server-port-handler" aria-label="Permalink to &quot;\`http.server(port, handler?)\`&quot;">​</a></h2><p>Memulai HTTP server pada port tertentu. Mengembalikan server handle.</p><h3 id="tanpa-handler-default-response" tabindex="-1">Tanpa handler (default response) <a class="header-anchor" href="#tanpa-handler-default-response" aria-label="Permalink to &quot;Tanpa handler (default response)&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>server = http.server(8080)</span></span>
<span class="line"><span>print(&quot;Server started on port 8080&quot;)</span></span></code></pre></div><h3 id="dengan-handler" tabindex="-1">Dengan handler <a class="header-anchor" href="#dengan-handler" aria-label="Permalink to &quot;Dengan handler&quot;">​</a></h3><p>Handler menerima objek <code>req</code> (request) dan <code>res</code> (response):</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>handler(req, res) {</span></span>
<span class="line"><span>  res.body = &quot;Hello &quot; + req.path + &quot;!&quot;</span></span>
<span class="line"><span>  return res</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>server = http.server(8080, handler)</span></span></code></pre></div><p><strong>Request object:</strong></p><ul><li><code>req.method</code> — HTTP method (GET, POST, dll)</li><li><code>req.path</code> — URL path</li><li><code>req.headers</code> — raw headers string</li><li><code>req.body</code> — request body</li></ul><p><strong>Response object:</strong></p><ul><li><code>res.status</code> — status code (default: 200)</li><li><code>res.body</code> — response body</li><li><code>res.setHeader(key, value)</code> — set response header</li><li><code>res.json(obj)</code> — serialize objek ke JSON</li></ul><h3 id="contoh-handler-lengkap" tabindex="-1">Contoh handler lengkap <a class="header-anchor" href="#contoh-handler-lengkap" aria-label="Permalink to &quot;Contoh handler lengkap&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>handler(req, res) {</span></span>
<span class="line"><span>  res.setHeader(&quot;X-Server&quot;, &quot;rupa&quot;)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  if req.path == &quot;/api/data&quot;:</span></span>
<span class="line"><span>    res.json({ status: &quot;ok&quot;, data: [1, 2, 3] })</span></span>
<span class="line"><span>  else:</span></span>
<span class="line"><span>    res.body = &quot;Welcome to Rupa HTTP Server!&quot;</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  return res</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>server = http.server(8080, handler)</span></span></code></pre></div><h2 id="http-stop-handle" tabindex="-1"><code>http.stop(handle)</code> <a class="header-anchor" href="#http-stop-handle" aria-label="Permalink to &quot;\`http.stop(handle)\`&quot;">​</a></h2><p>Menghentikan server yang sedang berjalan.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>http.stop(server)</span></span>
<span class="line"><span>print(&quot;Server stopped&quot;)</span></span></code></pre></div><h2 id="http-request-method-url-data" tabindex="-1"><code>http.request(method, url, data?)</code> <a class="header-anchor" href="#http-request-method-url-data" aria-label="Permalink to &quot;\`http.request(method, url, data?)\`&quot;">​</a></h2><p>Melakukan HTTP request. Method: <code>&quot;GET&quot;</code>, <code>&quot;POST&quot;</code>, <code>&quot;PUT&quot;</code>, <code>&quot;DELETE&quot;</code>.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// GET request</span></span>
<span class="line"><span>response = http.request(&quot;GET&quot;, &quot;https://api.example.com/data&quot;)</span></span>
<span class="line"><span>print(response)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// POST request</span></span>
<span class="line"><span>response = http.request(&quot;POST&quot;, &quot;https://api.example.com/data&quot;, &quot;name=Rupa&quot;)</span></span>
<span class="line"><span>print(response)</span></span></code></pre></div><h2 id="http-get-url" tabindex="-1"><code>http.get(url)</code> <a class="header-anchor" href="#http-get-url" aria-label="Permalink to &quot;\`http.get(url)\`&quot;">​</a></h2><p>Shortcut untuk GET request.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>response = http.get(&quot;https://api.example.com/data&quot;)</span></span>
<span class="line"><span>print(response)</span></span></code></pre></div><h2 id="http-post-url-data" tabindex="-1"><code>http.post(url, data)</code> <a class="header-anchor" href="#http-post-url-data" aria-label="Permalink to &quot;\`http.post(url, data)\`&quot;">​</a></h2><p>Shortcut untuk POST request.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>response = http.post(&quot;https://api.example.com/data&quot;, &quot;name=Rupa&quot;)</span></span>
<span class="line"><span>print(response)</span></span></code></pre></div><h2 id="contoh-http-server-sederhana" tabindex="-1">Contoh: HTTP Server Sederhana <a class="header-anchor" href="#contoh-http-server-sederhana" aria-label="Permalink to &quot;Contoh: HTTP Server Sederhana&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span>
<span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// Start server</span></span>
<span class="line"><span>server = http.server(8080)</span></span>
<span class="line"><span>print(&quot;Server running on http://localhost:8080&quot;)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// Keep server running</span></span>
<span class="line"><span>thread.sleep(5000)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// Stop server</span></span>
<span class="line"><span>http.stop(server)</span></span>
<span class="line"><span>print(&quot;Server stopped&quot;)</span></span></code></pre></div><h2 id="contoh-http-client" tabindex="-1">Contoh: HTTP Client <a class="header-anchor" href="#contoh-http-client" aria-label="Permalink to &quot;Contoh: HTTP Client&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import http from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// Fetch data from API</span></span>
<span class="line"><span>response = http.get(&quot;https://jsonplaceholder.typicode.com/todos/1&quot;)</span></span>
<span class="line"><span>print(response)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// Send data</span></span>
<span class="line"><span>response = http.post(&quot;https://jsonplaceholder.typicode.com/posts&quot;, &quot;title=foo&amp;body=bar&amp;userId=1&quot;)</span></span>
<span class="line"><span>print(response)</span></span></code></pre></div><h2 id="catatan" tabindex="-1">Catatan <a class="header-anchor" href="#catatan" aria-label="Permalink to &quot;Catatan&quot;">​</a></h2><ul><li>Server berjalan di background thread</li><li>Client menggunakan <code>curl</code> atau <code>wget</code> (harus terinstall)</li><li>Response berupa string (body response)</li><li>Timeout default: 30 detik</li></ul>`,34)])])}const q=s(t,[["render",o]]);export{u as __pageData,q as default};
