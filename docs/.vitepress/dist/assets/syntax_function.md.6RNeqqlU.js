import{_ as n,o as s,c as p,a0 as e}from"./chunks/framework.BpJS36ta.js";const h=JSON.parse('{"title":"Function","description":"","frontmatter":{},"headers":[],"relativePath":"syntax/function.md","filePath":"syntax/function.md"}'),i={name:"syntax/function.md"};function t(l,a,o,c,d,r){return s(),p("div",null,[...a[0]||(a[0]=[e(`<h1 id="function" tabindex="-1">Function <a class="header-anchor" href="#function" aria-label="Permalink to &quot;Function&quot;">​</a></h1><h2 id="apa-yang-bisa-ditulis" tabindex="-1">Apa yang bisa ditulis? <a class="header-anchor" href="#apa-yang-bisa-ditulis" aria-label="Permalink to &quot;Apa yang bisa ditulis?&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x: number, y: number) {</span></span>
<span class="line"><span>    return x + y</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Pemanggilan:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(1, 2)</span></span></code></pre></div><p>Function tanpa parameter:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>greet() {</span></span>
<span class="line"><span>    print(&quot;Hello!&quot;)</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span>greet()</span></span></code></pre></div><p>Function dengan object return:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>makePoint(px, py) {</span></span>
<span class="line"><span>    return { x: px, y: py }</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span>p = makePoint(10, 20)</span></span>
<span class="line"><span>print(p.x)</span></span></code></pre></div><h2 id="kapan-digunakan" tabindex="-1">Kapan digunakan? <a class="header-anchor" href="#kapan-digunakan" aria-label="Permalink to &quot;Kapan digunakan?&quot;">​</a></h2><p>Gunakan declaration ketika ingin memberi nama pada logic yang dapat dipanggil kembali. Gunakan parameter annotation ketika parameter memiliki type yang diharapkan.</p><h2 id="apa-hasilnya" tabindex="-1">Apa hasilnya? <a class="header-anchor" href="#apa-hasilnya" aria-label="Permalink to &quot;Apa hasilnya?&quot;">​</a></h2><p>Declaration memiliki nama, parameter, dan body. Identifier yang diikuti argument tetapi tidak membentuk declaration digunakan sebagai call.</p><h3 id="contoh-execution" tabindex="-1">Contoh execution <a class="header-anchor" href="#contoh-execution" aria-label="Permalink to &quot;Contoh execution&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>add(x: number, y: number) {</span></span>
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
<span class="line"><span>print(p.x, p.y)</span></span></code></pre></div><p>Output: <code>10 20</code></p>`,20)])])}const g=n(i,[["render",t]]);export{h as __pageData,g as default};
