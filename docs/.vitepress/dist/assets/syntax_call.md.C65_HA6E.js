import{_ as n,o as s,c as p,a0 as e}from"./chunks/framework.BpJS36ta.js";const h=JSON.parse('{"title":"Call","description":"","frontmatter":{},"headers":[],"relativePath":"syntax/call.md","filePath":"syntax/call.md"}'),l={name:"syntax/call.md"};function t(i,a,o,c,d,r){return s(),p("div",null,[...a[0]||(a[0]=[e(`<h1 id="call" tabindex="-1">Call <a class="header-anchor" href="#call" aria-label="Permalink to &quot;Call&quot;">​</a></h1><h2 id="apa-yang-bisa-ditulis" tabindex="-1">Apa yang bisa ditulis? <a class="header-anchor" href="#apa-yang-bisa-ditulis" aria-label="Permalink to &quot;Apa yang bisa ditulis?&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(1, 2)</span></span>
<span class="line"><span>print(&quot;hello&quot;)</span></span>
<span class="line"><span>greet()</span></span></code></pre></div><p>Nested call:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>double(n) {</span></span>
<span class="line"><span>    return n * 2</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span>triple(n) {</span></span>
<span class="line"><span>    return n * 3</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span>val = double(triple(5))</span></span></code></pre></div><h2 id="kapan-digunakan" tabindex="-1">Kapan digunakan? <a class="header-anchor" href="#kapan-digunakan" aria-label="Permalink to &quot;Kapan digunakan?&quot;">​</a></h2><p>Gunakan call untuk menjalankan function yang sudah dideklarasikan. Arguments dipisahkan koma.</p><h2 id="apa-hasilnya" tabindex="-1">Apa hasilnya? <a class="header-anchor" href="#apa-hasilnya" aria-label="Permalink to &quot;Apa hasilnya?&quot;">​</a></h2><p>Call mengevaluasi arguments dan menjalankan function body. Hasilnya adalah return value dari function.</p><h3 id="contoh-execution" tabindex="-1">Contoh execution <a class="header-anchor" href="#contoh-execution" aria-label="Permalink to &quot;Contoh execution&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x, y) {</span></span>
<span class="line"><span>    return x + y</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>result = add(1, 2)</span></span>
<span class="line"><span>print(result)</span></span></code></pre></div><p>Output: <code>3</code></p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&quot;hello world&quot;)</span></span></code></pre></div><p>Output: <code>hello world</code></p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>double(n) {</span></span>
<span class="line"><span>    return n * 2</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span>triple(n) {</span></span>
<span class="line"><span>    return n * 3</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span>val = double(triple(5))</span></span>
<span class="line"><span>print(val)</span></span></code></pre></div><p>Output: <code>30</code></p>`,16)])])}const b=n(l,[["render",t]]);export{h as __pageData,b as default};
