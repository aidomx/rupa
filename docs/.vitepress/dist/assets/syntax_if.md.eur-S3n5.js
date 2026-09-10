import{_ as n,o as s,c as e,a0 as p}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"If","description":"","frontmatter":{},"headers":[],"relativePath":"syntax/if.md","filePath":"syntax/if.md"}'),t={name:"syntax/if.md"};function i(o,a,l,c,u,d){return s(),e("div",null,[...a[0]||(a[0]=[p(`<h1 id="if" tabindex="-1">If <a class="header-anchor" href="#if" aria-label="Permalink to &quot;If&quot;">​</a></h1><h2 id="apa-yang-bisa-ditulis" tabindex="-1">Apa yang bisa ditulis? <a class="header-anchor" href="#apa-yang-bisa-ditulis" aria-label="Permalink to &quot;Apa yang bisa ditulis?&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if x &gt; 0: print(true)</span></span></code></pre></div><p>Dengan block dan branch:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>if x &gt; 0 {</span></span>
<span class="line"><span>    print(&quot;positive&quot;)</span></span>
<span class="line"><span>} elseif x == 0 {</span></span>
<span class="line"><span>    print(&quot;zero&quot;)</span></span>
<span class="line"><span>} else {</span></span>
<span class="line"><span>    print(&quot;negative&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><h2 id="kapan-digunakan" tabindex="-1">Kapan digunakan? <a class="header-anchor" href="#kapan-digunakan" aria-label="Permalink to &quot;Kapan digunakan?&quot;">​</a></h2><p>Gunakan <code>if</code> untuk menjalankan body berdasarkan condition. Gunakan <code>elseif</code> untuk condition lanjutan dan <code>else</code> untuk kondisi selain branch sebelumnya.</p><h2 id="apa-hasilnya" tabindex="-1">Apa hasilnya? <a class="header-anchor" href="#apa-hasilnya" aria-label="Permalink to &quot;Apa hasilnya?&quot;">​</a></h2><p>Hanya body dari branch yang sesuai condition yang dipilih. Body dapat berupa satu statement setelah <code>:</code> atau block <code>{ ... }</code>.</p><h3 id="contoh-execution" tabindex="-1">Contoh execution <a class="header-anchor" href="#contoh-execution" aria-label="Permalink to &quot;Contoh execution&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>x = 10</span></span>
<span class="line"><span></span></span>
<span class="line"><span>if x &gt; 0 {</span></span>
<span class="line"><span>    print(&quot;positive&quot;)</span></span>
<span class="line"><span>} elseif x == 0 {</span></span>
<span class="line"><span>    print(&quot;zero&quot;)</span></span>
<span class="line"><span>} else {</span></span>
<span class="line"><span>    print(&quot;negative&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Output: <code>positive</code></p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>status = 404</span></span>
<span class="line"><span></span></span>
<span class="line"><span>case status =&gt; {</span></span>
<span class="line"><span>    200: print(&quot;success&quot;)</span></span>
<span class="line"><span>    404: print(&quot;not found&quot;)</span></span>
<span class="line"><span>    500: print(&quot;server error&quot;)</span></span>
<span class="line"><span>    default: print(&quot;unknown&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Output: <code>not found</code></p>`,14)])])}const b=n(t,[["render",i]]);export{h as __pageData,b as default};
