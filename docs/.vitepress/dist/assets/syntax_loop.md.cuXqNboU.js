import{_ as s,o as n,c as p,a0 as e}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Loop","description":"","frontmatter":{},"headers":[],"relativePath":"syntax/loop.md","filePath":"syntax/loop.md"}'),i={name:"syntax/loop.md"};function t(l,a,o,c,d,u){return n(),p("div",null,[...a[0]||(a[0]=[e(`<h1 id="loop" tabindex="-1">Loop <a class="header-anchor" href="#loop" aria-label="Permalink to &quot;Loop&quot;">​</a></h1><h2 id="apa-yang-bisa-ditulis" tabindex="-1">Apa yang bisa ditulis? <a class="header-anchor" href="#apa-yang-bisa-ditulis" aria-label="Permalink to &quot;Apa yang bisa ditulis?&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>i = 10</span></span>
<span class="line"><span>for i: print(i)</span></span></code></pre></div><p>With condition:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>for i &lt; 10: print(i)</span></span></code></pre></div><p>With block:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>for i &lt; 10 {</span></span>
<span class="line"><span>    print(i)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Reverse loop:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>rev i &gt; 0 {</span></span>
<span class="line"><span>    print(i)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>While loop:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x = 0</span></span>
<span class="line"><span>while x &lt; 10 {</span></span>
<span class="line"><span>    print(x)</span></span>
<span class="line"><span>    x++</span></span>
<span class="line"><span>}</span></span></code></pre></div><h2 id="kapan-digunakan" tabindex="-1">Kapan digunakan? <a class="header-anchor" href="#kapan-digunakan" aria-label="Permalink to &quot;Kapan digunakan?&quot;">​</a></h2><p>Gunakan <code>for</code> untuk iterasi maju, <code>rev</code> untuk iterasi mundur, dan <code>while</code> untuk iterasi berdasarkan condition.</p><h2 id="apa-hasilnya" tabindex="-1">Apa hasilnya? <a class="header-anchor" href="#apa-hasilnya" aria-label="Permalink to &quot;Apa hasilnya?&quot;">​</a></h2><p>Loop menjalankan body berulang kali sampai condition tidak terpenuhi. Gunakan <code>break</code> untuk keluar dan <code>continue</code> untuk skip iterasi.</p><h3 id="contoh-execution" tabindex="-1">Contoh execution <a class="header-anchor" href="#contoh-execution" aria-label="Permalink to &quot;Contoh execution&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>for i &lt; 5 {</span></span>
<span class="line"><span>    print(i)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Output:</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>0</span></span>
<span class="line"><span>1</span></span>
<span class="line"><span>2</span></span>
<span class="line"><span>3</span></span>
<span class="line"><span>4</span></span></code></pre></div><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x = 0</span></span>
<span class="line"><span>while x &lt; 5 {</span></span>
<span class="line"><span>    print(x)</span></span>
<span class="line"><span>    x++</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Output:</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>0</span></span>
<span class="line"><span>1</span></span>
<span class="line"><span>2</span></span>
<span class="line"><span>3</span></span>
<span class="line"><span>4</span></span></code></pre></div><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>for i &lt; 10 {</span></span>
<span class="line"><span>    if i == 3: continue</span></span>
<span class="line"><span>    if i == 7: break</span></span>
<span class="line"><span>    print(i)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Output:</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>0</span></span>
<span class="line"><span>1</span></span>
<span class="line"><span>2</span></span>
<span class="line"><span>4</span></span>
<span class="line"><span>5</span></span>
<span class="line"><span>6</span></span></code></pre></div>`,25)])])}const g=s(i,[["render",t]]);export{h as __pageData,g as default};
