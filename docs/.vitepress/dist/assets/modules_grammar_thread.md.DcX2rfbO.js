import{_ as e,o as s,c as n,a0 as t}from"./chunks/framework.BpJS36ta.js";const u=JSON.parse('{"title":"Thread Module Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"modules/grammar/thread.md","filePath":"modules/grammar/thread.md"}'),p={name:"modules/grammar/thread.md"};function l(i,a,r,d,c,o){return s(),n("div",null,[...a[0]||(a[0]=[t(`<h1 id="thread-module-grammar" tabindex="-1">Thread Module Grammar <a class="header-anchor" href="#thread-module-grammar" aria-label="Permalink to &quot;Thread Module Grammar&quot;">​</a></h1><p>Module <code>thread</code> diimpor dari root namespace Rupa:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>import thread from rupa</span></span></code></pre></div><h2 id="ast-structure" tabindex="-1">AST Structure <a class="header-anchor" href="#ast-structure" aria-label="Permalink to &quot;AST Structure&quot;">​</a></h2><h3 id="thread-sleep-ms" tabindex="-1"><code>thread.sleep(ms)</code> <a class="header-anchor" href="#thread-sleep-ms" aria-label="Permalink to &quot;\`thread.sleep(ms)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: thread</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: sleep</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Number: 1000</span></span></code></pre></div><h3 id="thread-id" tabindex="-1"><code>thread.id()</code> <a class="header-anchor" href="#thread-id" aria-label="Permalink to &quot;\`thread.id()\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: thread</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: id</span></span></code></pre></div><h3 id="thread-count" tabindex="-1"><code>thread.count()</code> <a class="header-anchor" href="#thread-count" aria-label="Permalink to &quot;\`thread.count()\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: thread</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: count</span></span></code></pre></div><h3 id="thread-create-fn" tabindex="-1"><code>thread.create(fn)</code> <a class="header-anchor" href="#thread-create-fn" aria-label="Permalink to &quot;\`thread.create(fn)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: thread</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: create</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Identifier: fn</span></span></code></pre></div><h3 id="thread-join-handle" tabindex="-1"><code>thread.join(handle)</code> <a class="header-anchor" href="#thread-join-handle" aria-label="Permalink to &quot;\`thread.join(handle)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: thread</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: join</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Identifier: handle</span></span></code></pre></div><h2 id="module-structure" tabindex="-1">Module Structure <a class="header-anchor" href="#module-structure" aria-label="Permalink to &quot;Module Structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Module: thread</span></span>
<span class="line"><span>├── sleep(ms)    — nanosleep wrapper</span></span>
<span class="line"><span>├── id()         — pthread_self hash</span></span>
<span class="line"><span>├── count()      — active thread count</span></span>
<span class="line"><span>├── create(fn)   — pthread_create wrapper</span></span>
<span class="line"><span>└── join(handle) — pthread_join wrapper</span></span></code></pre></div><h2 id="implementation-notes" tabindex="-1">Implementation Notes <a class="header-anchor" href="#implementation-notes" aria-label="Permalink to &quot;Implementation Notes&quot;">​</a></h2><ul><li>Thread state stored in static <code>threadTable[MAX_THREADS]</code></li><li><code>thread.create()</code> accepts <code>VALUE_NATIVE_FUNCTION</code> only</li><li><code>thread.join()</code> blocks until thread completes</li><li>Thread results stored in <code>ThreadEntry.result</code></li><li>Error handling via <code>ThreadEntry.has_error</code> and <code>error_msg</code></li></ul>`,18)])])}const m=e(p,[["render",l]]);export{u as __pageData,m as default};
