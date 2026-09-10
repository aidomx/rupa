import{_ as s,o as n,c as e,a0 as t}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Async Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/async.md","filePath":"grammar/async.md"}'),i={name:"grammar/async.md"};function p(l,a,d,c,o,r){return n(),e("div",null,[...a[0]||(a[0]=[t(`<h1 id="async-grammar" tabindex="-1">Async Grammar <a class="header-anchor" href="#async-grammar" aria-label="Permalink to &quot;Async Grammar&quot;">​</a></h1><p>Grammar async membentuk node async dari request dan konfigurasi <code>{loader, timeout}</code>.</p><h2 id="syntax" tabindex="-1">Syntax <a class="header-anchor" href="#syntax" aria-label="Permalink to &quot;Syntax&quot;">​</a></h2><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>async &lt;request&gt;</span></span>
<span class="line"><span>async &lt;request&gt; -&gt; {loader, timeout}</span></span></code></pre></div><p><code>-&gt;</code> di sini adalah grammar then biasa (lihat <a href="./then.html">then.md</a>), bukan block handler. Isi <code>{}</code> di sisi kanannya <strong>selalu</strong> pasangan <code>loader</code> dan <code>timeout</code>, dua slot dipisah koma — tidak ada bentuk lain (bukan block statement, bukan ekspresi tunggal).</p><ul><li><code>loader</code> — referensi identifier ke function yang sudah dideklarasikan terpisah (lihat <a href="./function.html">function.md</a>); dipanggil dengan <code>this = {status, data, error}</code> setiap kali status berubah.</li><li><code>timeout</code> — angka literal langsung (<code>{loader, 5000}</code>) atau referensi identifier ke variabel yang sudah didefinisikan (<code>{loader, timeout}</code> dengan <code>timeout = 5000</code> di baris lain).</li></ul><p><code>-&gt;</code> boleh tidak dituliskan sama sekali; request tetap dievaluasi, hanya tanpa loader/timeout (lihat <a href="./await.html">await.md</a> untuk cara mengambil hasilnya lewat <code>await</code>).</p><h2 id="async-tanpa-handler" tabindex="-1">Async tanpa handler <a class="header-anchor" href="#async-tanpa-handler" aria-label="Permalink to &quot;Async tanpa handler&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>users = async db.getUser()</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Assignment:</span></span>
<span class="line"><span>  Target:</span></span>
<span class="line"><span>    Identifier: users</span></span>
<span class="line"><span>  Value:</span></span>
<span class="line"><span>    Async:</span></span>
<span class="line"><span>      Request:</span></span>
<span class="line"><span>        Call:</span></span>
<span class="line"><span>          Callee:</span></span>
<span class="line"><span>            Member:</span></span>
<span class="line"><span>              Object:</span></span>
<span class="line"><span>                Identifier: db</span></span>
<span class="line"><span>              Member:</span></span>
<span class="line"><span>                Identifier: getUser</span></span></code></pre></div><h2 id="async-dengan-loader-dan-timeout" tabindex="-1">Async dengan loader dan timeout <a class="header-anchor" href="#async-dengan-loader-dan-timeout" aria-label="Permalink to &quot;Async dengan loader dan timeout&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>loader() {</span></span>
<span class="line"><span>  if this.status == AWAIT:</span></span>
<span class="line"><span>    print(&quot;loading...\\n&quot;)</span></span>
<span class="line"><span>  else if this.status == SUCCESS:</span></span>
<span class="line"><span>    print(this.data, &quot;\\n&quot;)</span></span>
<span class="line"><span>  else:</span></span>
<span class="line"><span>    print(this.error, &quot;\\n&quot;)</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>async dbtest.getUser() -&gt; {loader, 5000}</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Async:</span></span>
<span class="line"><span>  Request:</span></span>
<span class="line"><span>    Call:</span></span>
<span class="line"><span>      Callee:</span></span>
<span class="line"><span>        Member:</span></span>
<span class="line"><span>          Object:</span></span>
<span class="line"><span>            Identifier: dbtest</span></span>
<span class="line"><span>          Member:</span></span>
<span class="line"><span>            Identifier: getUser</span></span>
<span class="line"><span>  Loader:</span></span>
<span class="line"><span>    Identifier: loader</span></span>
<span class="line"><span>  TimeoutId:</span></span>
<span class="line"><span>    Number: 5000</span></span></code></pre></div><p>Timeout juga boleh berupa referensi variabel alih-alih angka literal langsung:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>timeout = 5000</span></span>
<span class="line"><span>async dbtest.getUser() -&gt; {loader, timeout}</span></span></code></pre></div><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Async:</span></span>
<span class="line"><span>  Request: ...</span></span>
<span class="line"><span>  Loader:</span></span>
<span class="line"><span>    Identifier: loader</span></span>
<span class="line"><span>  TimeoutId:</span></span>
<span class="line"><span>    Identifier: timeout</span></span></code></pre></div><h2 id="async-structure" tabindex="-1">Async structure <a class="header-anchor" href="#async-structure" aria-label="Permalink to &quot;Async structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Async</span></span>
<span class="line"><span>├── Request    (expression yang dievaluasi)</span></span>
<span class="line"><span>├── Loader     (identifier reference ke loader function, -1 jika tidak ada)</span></span>
<span class="line"><span>└── TimeoutId  (Number atau Identifier reference, -1 jika tidak ada)</span></span></code></pre></div><h2 id="how-it-works" tabindex="-1">How it works <a class="header-anchor" href="#how-it-works" aria-label="Permalink to &quot;How it works&quot;">​</a></h2><ol><li>Request dievaluasi secara eager.</li><li>Jika <code>loader</code> ada, dipanggil dengan <code>this = {status: AWAIT, data: null, error: null}</code> sebelum request dievaluasi.</li><li>Setelah request selesai (atau timeout terlampaui), <code>loader</code> dipanggil lagi dengan <code>this = {status: SUCCESS|ERROR, data, error}</code>.</li><li>Hasil akhir async expression adalah handle <code>{status, data, error}</code> yang sama — bisa ditangkap lewat assignment dan dibaca lagi lewat <code>await</code> (lihat <a href="./await.html">await.md</a>).</li></ol><p>Karena <code>loader</code> bisa dipanggil lebih dari sekali (fase AWAIT lalu fase SUCCESS/ERROR), loader idealnya memeriksa <code>this.status</code> untuk membedakan setiap pemanggilan — seperti contoh di atas.</p>`,25)])])}const g=s(i,[["render",p]]);export{h as __pageData,g as default};
