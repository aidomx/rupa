import{_ as n,o as s,c as e,a0 as t}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Async","description":"","frontmatter":{},"headers":[],"relativePath":"syntax/async.md","filePath":"syntax/async.md"}'),i={name:"syntax/async.md"};function p(l,a,d,o,r,c){return s(),e("div",null,[...a[0]||(a[0]=[t(`<h1 id="async" tabindex="-1">Async <a class="header-anchor" href="#async" aria-label="Permalink to &quot;Async&quot;">​</a></h1><h2 id="apa-yang-bisa-ditulis" tabindex="-1">Apa yang bisa ditulis? <a class="header-anchor" href="#apa-yang-bisa-ditulis" aria-label="Permalink to &quot;Apa yang bisa ditulis?&quot;">​</a></h2><h3 id="tanpa-loader-timeout" tabindex="-1">Tanpa loader/timeout <a class="header-anchor" href="#tanpa-loader-timeout" aria-label="Permalink to &quot;Tanpa loader/timeout&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>users = async db.getUser()</span></span></code></pre></div><h3 id="dengan-loader-dan-timeout-angka-langsung" tabindex="-1">Dengan loader dan timeout (angka langsung) <a class="header-anchor" href="#dengan-loader-dan-timeout-angka-langsung" aria-label="Permalink to &quot;Dengan loader dan timeout (angka langsung)&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>loader() {</span></span>
<span class="line"><span>  if this.status == AWAIT:</span></span>
<span class="line"><span>    print(&quot;loading...&quot;)</span></span>
<span class="line"><span>  else if this.status == SUCCESS:</span></span>
<span class="line"><span>    print(this.data)</span></span>
<span class="line"><span>  else:</span></span>
<span class="line"><span>    print(this.error)</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>async db.getUser() -&gt; {loader, 5000}</span></span></code></pre></div><h3 id="dengan-loader-dan-timeout-referensi-variabel" tabindex="-1">Dengan loader dan timeout (referensi variabel) <a class="header-anchor" href="#dengan-loader-dan-timeout-referensi-variabel" aria-label="Permalink to &quot;Dengan loader dan timeout (referensi variabel)&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>loader() {</span></span>
<span class="line"><span>  if this.status == SUCCESS:</span></span>
<span class="line"><span>    print(this.data)</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>timeout = 5000</span></span>
<span class="line"><span>async db.getUser() -&gt; {loader, timeout}</span></span></code></pre></div><h2 id="kapan-digunakan" tabindex="-1">Kapan digunakan? <a class="header-anchor" href="#kapan-digunakan" aria-label="Permalink to &quot;Kapan digunakan?&quot;">​</a></h2><p>Gunakan <code>async</code> untuk menjalankan operasi yang hasilnya ingin ditangani lewat pola status (<code>AWAIT</code> → <code>SUCCESS</code>/<code>ERROR</code>), bukan cuma nilai balik biasa.</p><p><code>-&gt;</code> (then) di sini punya satu-satunya bentuk yang valid: <code>{loader, timeout}</code>.</p><ul><li><code>loader</code> — nama function yang <strong>sudah dideklarasikan terpisah</strong> sebelumnya (lihat <a href="./function.html">function.md</a>). Bukan block statement, bukan ekspresi inline.</li><li><code>timeout</code> — angka literal langsung (<code>{loader, 5000}</code>) atau nama variabel yang sudah diisi angka (<code>{loader, timeout}</code>).</li></ul><p><code>-&gt;</code> boleh dihilangkan sama sekali kalau kamu hanya butuh menyimpan handle-nya dulu dan membaca hasilnya belakangan lewat <code>await</code> (lihat <a href="./await.html">await.md</a>).</p><h2 id="hasilnya" tabindex="-1">Hasilnya? <a class="header-anchor" href="#hasilnya" aria-label="Permalink to &quot;Hasilnya?&quot;">​</a></h2><p><code>async</code> mengembalikan handle dengan status <code>{status, data, error}</code>.</p><h3 id="status" tabindex="-1">Status <a class="header-anchor" href="#status" aria-label="Permalink to &quot;Status&quot;">​</a></h3><table tabindex="0"><thead><tr><th>Status</th><th>Arti</th></tr></thead><tbody><tr><td><code>AWAIT</code></td><td>Request sedang berjalan</td></tr><tr><td><code>SUCCESS</code></td><td>Request berhasil, <code>data</code> tersedia</td></tr><tr><td><code>ERROR</code></td><td>Request gagal atau timeout</td></tr></tbody></table><p>Kalau <code>loader</code> diberikan, ia dipanggil <strong>dua kali</strong>: sekali dengan <code>this.status == AWAIT</code> (sebelum request berjalan), lalu sekali lagi dengan <code>this.status == SUCCESS</code> atau <code>this.status == ERROR</code> (setelah request selesai atau timeout terlampaui). Karena itu loader biasanya memeriksa <code>this.status</code> untuk membedakan kedua pemanggilan tersebut.</p><h3 id="contoh-eksekusi" tabindex="-1">Contoh eksekusi <a class="header-anchor" href="#contoh-eksekusi" aria-label="Permalink to &quot;Contoh eksekusi&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import dbtest from rupa</span></span>
<span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>loader() {</span></span>
<span class="line"><span>  if this.status == AWAIT:</span></span>
<span class="line"><span>    print(&quot;loading...\\n&quot;)</span></span>
<span class="line"><span>  else if this.status == SUCCESS:</span></span>
<span class="line"><span>    print(this.data, &quot;\\n&quot;)</span></span>
<span class="line"><span>  else:</span></span>
<span class="line"><span>    print(this.error, &quot;\\n&quot;)</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>slowGetUsers() {</span></span>
<span class="line"><span>  thread.sleep(200)</span></span>
<span class="line"><span>  return dbtest.getUser()</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// Request cepat, timeout longgar → SUCCESS</span></span>
<span class="line"><span>async dbtest.getUser() -&gt; {loader, 5000}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>// Request lambat, timeout ketat → ERROR</span></span>
<span class="line"><span>async slowGetUsers() -&gt; {loader, 50}</span></span></code></pre></div><p>Output:</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>loading...</span></span>
<span class="line"><span>[{id: 1, name: admin}, {id: 2, name: user}]</span></span>
<span class="line"><span>loading...</span></span>
<span class="line"><span>Request timed out</span></span></code></pre></div><h3 id="contoh-dengan-await" tabindex="-1">Contoh dengan await <a class="header-anchor" href="#contoh-dengan-await" aria-label="Permalink to &quot;Contoh dengan await&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import dbtest from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>users = async dbtest.getUser()</span></span>
<span class="line"><span>d = (await users).data</span></span>
<span class="line"><span>print(d, &quot;\\n&quot;)</span></span></code></pre></div><p><code>await</code> mengembalikan handle yang sudah diperbarui; <code>.data</code> mengambil hasil darinya. Detail lengkap ada di <a href="./await.html">await.md</a>.</p>`,25)])])}const g=n(i,[["render",p]]);export{h as __pageData,g as default};
