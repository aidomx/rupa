import { defineConfig } from 'vitepress'

export default defineConfig({
  base: '/rupa/',
  title: 'Rupa',
  description: 'Satu Bahasa, Banyak Cara Bicara',
  
  head: [
    ['link', { rel: 'icon', type: 'image/svg+xml', href: '/rupa/logo.svg' }]
  ],

  themeConfig: {
    nav: [
      { text: 'Beranda', link: '/' },
      { text: 'Panduan', link: '/guide/getting-started' },
      { text: 'Syntax', link: '/syntax/' },
      { text: 'Grammar', link: '/grammar/' },
      { text: 'Modules', link: '/modules/syntax/' },
      { text: 'TODO', link: '/TODO' }
    ],

    sidebar: {
      '/guide/': [
        {
          text: 'Panduan',
          items: [
            { text: 'Memulai', link: '/guide/getting-started' }
          ]
        }
      ],
      '/syntax/': [
        {
          text: 'Syntax',
          items: [
            { text: 'Index', link: '/syntax/' },
            { text: 'Basics', link: '/syntax/syntax' },
            { text: 'Comment', link: '/syntax/comment' },
            { text: 'Main', link: '/syntax/main' },
            { text: 'Literal', link: '/syntax/literal' },
            { text: 'Print', link: '/syntax/print' },
            { text: 'Assignment', link: '/syntax/assignment' },
            { text: 'Expression', link: '/syntax/expression' },
            { text: 'Update', link: '/syntax/update' },
            { text: 'String', link: '/syntax/string' },
            { text: 'Array', link: '/syntax/array' },
            { text: 'Object', link: '/syntax/object' },
            { text: 'If/Else', link: '/syntax/if' },
            { text: 'Block', link: '/syntax/block' },
            { text: 'Function', link: '/syntax/function' },
            { text: 'Return', link: '/syntax/return' },
            { text: 'Loop', link: '/syntax/loop' },
            { text: 'Case', link: '/syntax/case' },
            { text: 'Call', link: '/syntax/call' },
            { text: 'Struct', link: '/syntax/struct' },
            { text: 'Class', link: '/syntax/class' },
            { text: 'Annotation', link: '/syntax/annotation' },
            { text: 'Control', link: '/syntax/control' },
            { text: 'Fallback', link: '/syntax/fallback' },
            { text: 'Then', link: '/syntax/then' },
            { text: 'Async', link: '/syntax/async' },
            { text: 'Await', link: '/syntax/await' },
            { text: 'Builtins', link: '/syntax/builtins' },
            { text: 'Module', link: '/syntax/module' },
            { text: 'Import', link: '/syntax/import' },
            { text: 'Export', link: '/syntax/export' },
            { text: 'View', link: '/syntax/view' }
          ]
        }
      ],
      '/grammar/': [
        {
          text: 'Grammar',
          items: [
            { text: 'Index', link: '/grammar/' },
            { text: 'Overview', link: '/grammar/grammar' },
            { text: 'Comment', link: '/grammar/comment' },
            { text: 'Module', link: '/grammar/module' },
            { text: 'Import', link: '/grammar/import' },
            { text: 'Export', link: '/grammar/export' },
            { text: 'Assignment', link: '/grammar/assignment' },
            { text: 'Expression', link: '/grammar/expression' },
            { text: 'Literal', link: '/grammar/literal' },
            { text: 'Array', link: '/grammar/array' },
            { text: 'Object', link: '/grammar/object' },
            { text: 'Member', link: '/grammar/member' },
            { text: 'Function', link: '/grammar/function' },
            { text: 'If', link: '/grammar/if' },
            { text: 'Block', link: '/grammar/block' },
            { text: 'Loop', link: '/grammar/loop' },
            { text: 'Case', link: '/grammar/case' },
            { text: 'Call', link: '/grammar/call' },
            { text: 'Struct', link: '/grammar/struct' },
            { text: 'Annotation', link: '/grammar/annotation' },
            { text: 'Memory', link: '/grammar/memory' },
            { text: 'Update', link: '/grammar/update' },
            { text: 'Control', link: '/grammar/control' },
            { text: 'Fallback', link: '/grammar/fallback' },
            { text: 'Then', link: '/grammar/then' },
            { text: 'Return', link: '/grammar/return' },
            { text: 'Print', link: '/grammar/print' },
            { text: 'Async', link: '/grammar/async' },
            { text: 'Await', link: '/grammar/await' },
            { text: 'HTTP', link: '/grammar/http' }
          ]
        }
      ],
      '/modules/': [
        {
          text: 'Modules',
          items: [
            { text: 'Index', link: '/modules/syntax/' },
            { text: 'Math', link: '/modules/syntax/math' },
            { text: 'OS', link: '/modules/syntax/os' },
            { text: 'IO', link: '/modules/syntax/io' },
            { text: 'JSON', link: '/modules/syntax/json' },
            { text: 'String', link: '/modules/syntax/string' },
            { text: 'Thread', link: '/modules/syntax/thread' },
            { text: 'HTTP', link: '/modules/syntax/http' },
            { text: 'DateTime', link: '/modules/syntax/datetime' },
            { text: 'Regex', link: '/modules/syntax/regex' },
            { text: 'Crypto', link: '/modules/syntax/crypto' },
            { text: 'Net', link: '/modules/syntax/net' },
            { text: 'Sys', link: '/modules/syntax/sys' },
            { text: 'Fs', link: '/modules/syntax/fs' },
            { text: 'Database', link: '/modules/syntax/database' }
          ]
        },
        {
          text: 'Module Grammar',
          collapsed: true,
          items: [
            { text: 'Index', link: '/modules/grammar/' },
            { text: 'Os', link: '/modules/grammar/os' },
            { text: 'Thread', link: '/modules/grammar/thread' },
            { text: 'HTTP', link: '/modules/grammar/http' },
            { text: 'DateTime', link: '/modules/grammar/datetime' },
            { text: 'Regex', link: '/modules/grammar/regex' },
            { text: 'Crypto', link: '/modules/grammar/crypto' },
            { text: 'Net', link: '/modules/grammar/net' }
          ]
        }
      ],
    },

    socialLinks: [
      { icon: 'github', link: 'https://github.com/aidomx/rupa' }
    ],

    footer: {
      message: 'Dirilis di bawah Lisensi MIT.',
      copyright: '© 2026 Rupa Language'
    },

    outline: {
      label: 'Di halaman ini'
    },

    docFooter: {
      prev: 'Sebelumnya',
      next: 'Berikutnya'
    }
  }
})
