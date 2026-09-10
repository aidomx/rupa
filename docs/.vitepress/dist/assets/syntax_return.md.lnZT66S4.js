import{_ as n,o as s,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Return","description":"","frontmatter":{},"headers":[],"relativePath":"syntax/return.md","filePath":"syntax/return.md"}'),t={name:"syntax/return.md"};function i(l,a,o,r,c,u){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="return" tabindex="-1">Return <a class="header-anchor" href="#return" aria-label="Permalink to &quot;Return&quot;">​</a></h1><h2 id="apa-yang-bisa-ditulis" tabindex="-1">Apa yang bisa ditulis? <a class="header-anchor" href="#apa-yang-bisa-ditulis" aria-label="Permalink to &quot;Apa yang bisa ditulis?&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x, y) {</span></span>
<span class="line"><span>    return x + y</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Return tanpa value:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>greet() {</span></span>
<span class="line"><span>    print(&quot;Hello!&quot;)</span></span>
<span class="line"><span>    return</span></span>
<span class="line"><span>}</span></span></code></pre></div><h2 id="kapan-digunakan" tabindex="-1">Kapan digunakan? <a class="header-anchor" href="#kapan-digunakan" aria-label="Permalink to &quot;Kapan digunakan?&quot;">​</a></h2><p>Gunakan <code>return</code> untuk mengembalikan value dari function. Return dapat digunakan di mana saja dalam function body.</p><h2 id="apa-hasilnya" tabindex="-1">Apa hasilnya? <a class="header-anchor" href="#apa-hasilnya" aria-label="Permalink to &quot;Apa hasilnya?&quot;">​</a></h2><p>Return menghentikan eksekusi function dan mengembalikan value ke pemanggil.</p><h3 id="contoh-execution" tabindex="-1">Contoh execution <a class="header-anchor" href="#contoh-execution" aria-label="Permalink to &quot;Contoh execution&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x, y) {</span></span>
<span class="line"><span>    return x + y</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>result = add(1, 2)</span></span>
<span class="line"><span>print(result)</span></span></code></pre></div><p>Output: <code>3</code></p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>double(n) {</span></span>
<span class="line"><span>    return n * 2</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>val = double(5)</span></span>
<span class="line"><span>print(val)</span></span></code></pre></div><p>Output: <code>10</code></p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>makePoint(px, py) {</span></span>
<span class="line"><span>    return { x: px, y: py }</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>p = makePoint(10, 20)</span></span>
<span class="line"><span>print(p.x, p.y)</span></span></code></pre></div><p>Output: <code>10 20</code></p>`,16)])])}const g=n(t,[["render",i]]);export{h as __pageData,g as default};
