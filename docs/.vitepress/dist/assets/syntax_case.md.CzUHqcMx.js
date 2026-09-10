import{_ as n,o as s,c as e,a0 as t}from"./chunks/framework.CShDTZbC.js";const h=JSON.parse('{"title":"Case","description":"","frontmatter":{},"headers":[],"relativePath":"syntax/case.md","filePath":"syntax/case.md"}'),p={name:"syntax/case.md"};function i(o,a,l,c,u,r){return s(),e("div",null,[...a[0]||(a[0]=[t(`<h1 id="case" tabindex="-1">Case <a class="header-anchor" href="#case" aria-label="Permalink to &quot;Case&quot;">​</a></h1><h2 id="apa-yang-bisa-ditulis" tabindex="-1">Apa yang bisa ditulis? <a class="header-anchor" href="#apa-yang-bisa-ditulis" aria-label="Permalink to &quot;Apa yang bisa ditulis?&quot;">​</a></h2><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>case status =&gt; {</span></span>
<span class="line"><span>    200: print(&quot;success&quot;)</span></span>
<span class="line"><span>    404: print(&quot;not found&quot;)</span></span>
<span class="line"><span>    500: print(&quot;server error&quot;)</span></span>
<span class="line"><span>    default: print(&quot;unknown&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><h2 id="kapan-digunakan" tabindex="-1">Kapan digunakan? <a class="header-anchor" href="#kapan-digunakan" aria-label="Permalink to &quot;Kapan digunakan?&quot;">​</a></h2><p>Gunakan <code>case</code> untuk pattern matching. Setiap case memiliki pattern dan body. Wildcard <code>default</code> menangkap semua case yang tidak terpenuhi.</p><h2 id="apa-hasilnya" tabindex="-1">Apa hasilnya? <a class="header-anchor" href="#apa-hasilnya" aria-label="Permalink to &quot;Apa hasilnya?&quot;">​</a></h2><p>Case mengevaluasi subject dan menjalankan body yang sesuai dengan pattern. Jika tidak ada yang cocok, wildcard dijalankan.</p><h3 id="contoh-execution" tabindex="-1">Contoh execution <a class="header-anchor" href="#contoh-execution" aria-label="Permalink to &quot;Contoh execution&quot;">​</a></h3><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>status = 404</span></span>
<span class="line"><span></span></span>
<span class="line"><span>case status =&gt; {</span></span>
<span class="line"><span>    200: print(&quot;success&quot;)</span></span>
<span class="line"><span>    404: print(&quot;not found&quot;)</span></span>
<span class="line"><span>    500: print(&quot;server error&quot;)</span></span>
<span class="line"><span>    default: print(&quot;unknown&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Output: <code>not found</code></p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>status = 999</span></span>
<span class="line"><span></span></span>
<span class="line"><span>case status =&gt; {</span></span>
<span class="line"><span>    200: print(&quot;success&quot;)</span></span>
<span class="line"><span>    404: print(&quot;not found&quot;)</span></span>
<span class="line"><span>    500: print(&quot;server error&quot;)</span></span>
<span class="line"><span>    default: print(&quot;unknown&quot;)</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Output: <code>unknown</code></p>`,12)])])}const g=n(p,[["render",i]]);export{h as __pageData,g as default};
