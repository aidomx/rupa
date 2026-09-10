import{_ as n,o as t,c as s,a0 as e}from"./chunks/framework.BpJS36ta.js";const u=JSON.parse('{"title":"Thread Module","description":"","frontmatter":{},"headers":[],"relativePath":"modules/syntax/thread.md","filePath":"modules/syntax/thread.md"}'),d={name:"modules/syntax/thread.md"};function p(r,a,i,l,h,o){return t(),s("div",null,[...a[0]||(a[0]=[e(`<h1 id="thread-module" tabindex="-1">Thread Module <a class="header-anchor" href="#thread-module" aria-label="Permalink to &quot;Thread Module&quot;">​</a></h1><p>Modul <code>thread</code> menyediakan fungsi untuk menjalankan kode secara paralel menggunakan system thread.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span></code></pre></div><h2 id="thread-sleep-ms" tabindex="-1"><code>thread.sleep(ms)</code> <a class="header-anchor" href="#thread-sleep-ms" aria-label="Permalink to &quot;\`thread.sleep(ms)\`&quot;">​</a></h2><p>Menghentikan thread selama beberapa milidetik.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>print(&quot;Start&quot;)</span></span>
<span class="line"><span>thread.sleep(1000)</span></span>
<span class="line"><span>print(&quot;After 1 second&quot;)</span></span></code></pre></div><table tabindex="0"><thead><tr><th>Parameter</th><th>Type</th><th>Deskripsi</th></tr></thead><tbody><tr><td><code>ms</code></td><td>number</td><td>Milidetik yang harus ditunggu</td></tr></tbody></table><h2 id="thread-id" tabindex="-1"><code>thread.id()</code> <a class="header-anchor" href="#thread-id" aria-label="Permalink to &quot;\`thread.id()\`&quot;">​</a></h2><p>Mengembalikan ID dari thread yang sedang berjalan.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>id = thread.id()</span></span>
<span class="line"><span>print(&quot;Thread ID: &quot; + id)</span></span></code></pre></div><table tabindex="0"><thead><tr><th>Return</th><th>Type</th><th>Deskripsi</th></tr></thead><tbody><tr><td>-</td><td>number</td><td>Hash dari thread ID saat ini</td></tr></tbody></table><h2 id="thread-count" tabindex="-1"><code>thread.count()</code> <a class="header-anchor" href="#thread-count" aria-label="Permalink to &quot;\`thread.count()\`&quot;">​</a></h2><p>Mengembalikan jumlah thread yang masih aktif (belum selesai).</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>print(&quot;Active threads: &quot; + thread.count())</span></span></code></pre></div><table tabindex="0"><thead><tr><th>Return</th><th>Type</th><th>Deskripsi</th></tr></thead><tbody><tr><td>-</td><td>number</td><td>Jumlah thread aktif</td></tr></tbody></table><h2 id="thread-create-fn" tabindex="-1"><code>thread.create(fn)</code> <a class="header-anchor" href="#thread-create-fn" aria-label="Permalink to &quot;\`thread.create(fn)\`&quot;">​</a></h2><p>Membuat thread baru yang menjalankan fungsi <code>fn</code>. Mengembalikan handle thread.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>myTask() {</span></span>
<span class="line"><span>  print(&quot;Running in background\\n&quot;)</span></span>
<span class="line"><span>  thread.sleep(500)</span></span>
<span class="line"><span>  print(&quot;Done!\\n&quot;)</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>handle = thread.create(myTask)</span></span>
<span class="line"><span>thread.join(handle)</span></span>
<span class="line"><span>print(&quot;Thread finished\\n&quot;)</span></span></code></pre></div><table tabindex="0"><thead><tr><th>Parameter</th><th>Type</th><th>Deskripsi</th></tr></thead><tbody><tr><td><code>fn</code></td><td>function</td><td>Fungsi native yang akan dijalankan di thread baru</td></tr></tbody></table><table tabindex="0"><thead><tr><th>Return</th><th>Type</th><th>Deskripsi</th></tr></thead><tbody><tr><td>-</td><td>number</td><td>Handle thread (untuk digunakan dengan <code>thread.join</code>)</td></tr></tbody></table><h2 id="thread-join-handle" tabindex="-1"><code>thread.join(handle)</code> <a class="header-anchor" href="#thread-join-handle" aria-label="Permalink to &quot;\`thread.join(handle)\`&quot;">​</a></h2><p>Menunggu thread selesai dan mengembalikan hasilnya.</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>compute() {</span></span>
<span class="line"><span>  return 42</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>handle = thread.create(compute)</span></span>
<span class="line"><span>result = thread.join(handle)</span></span>
<span class="line"><span>print(&quot;Result: &quot; + result + &quot;\\n&quot;)</span></span></code></pre></div><table tabindex="0"><thead><tr><th>Parameter</th><th>Type</th><th>Deskripsi</th></tr></thead><tbody><tr><td><code>handle</code></td><td>number</td><td>Handle dari <code>thread.create()</code></td></tr></tbody></table><table tabindex="0"><thead><tr><th>Return</th><th>Type</th><th>Deskripsi</th></tr></thead><tbody><tr><td>-</td><td>any</td><td>Nilai yang dikembalikan oleh fungsi thread</td></tr></tbody></table><h2 id="ringkasan" tabindex="-1">Ringkasan <a class="header-anchor" href="#ringkasan" aria-label="Permalink to &quot;Ringkasan&quot;">​</a></h2><table tabindex="0"><thead><tr><th>Fungsi</th><th>Parameter</th><th>Return</th><th>Deskripsi</th></tr></thead><tbody><tr><td><code>sleep(ms)</code></td><td>number</td><td>-</td><td>Tidur selama ms milidetik</td></tr><tr><td><code>id()</code></td><td>-</td><td>number</td><td>ID thread saat ini</td></tr><tr><td><code>count()</code></td><td>-</td><td>number</td><td>Jumlah thread aktif</td></tr><tr><td><code>create(fn)</code></td><td>function</td><td>number</td><td>Buat thread baru</td></tr><tr><td><code>join(handle)</code></td><td>number</td><td>any</td><td>Tunggu thread selesai</td></tr></tbody></table><h2 id="contoh-parallel-processing" tabindex="-1">Contoh: Parallel Processing <a class="header-anchor" href="#contoh-parallel-processing" aria-label="Permalink to &quot;Contoh: Parallel Processing&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span>
<span class="line"><span></span></span>
<span class="line"><span>taskA() {</span></span>
<span class="line"><span>  thread.sleep(500)</span></span>
<span class="line"><span>  return &quot;Task A done&quot;</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>taskB() {</span></span>
<span class="line"><span>  thread.sleep(300)</span></span>
<span class="line"><span>  return &quot;Task B done&quot;</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>a = thread.create(taskA)</span></span>
<span class="line"><span>b = thread.create(taskB)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>resultA = thread.join(a)</span></span>
<span class="line"><span>resultB = thread.join(b)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>print(resultA + &quot;\\n&quot;)</span></span>
<span class="line"><span>print(resultB + &quot;\\n&quot;)</span></span>
<span class="line"><span>print(&quot;Both tasks completed\\n&quot;)</span></span></code></pre></div>`,29)])])}const b=n(d,[["render",p]]);export{u as __pageData,b as default};
