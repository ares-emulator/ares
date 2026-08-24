import { themes as prismThemes } from 'prism-react-renderer';
import remarkGithubAdmonitionsToDirectives from "remark-github-admonitions-to-directives";
import RemarkLinkRewrite from 'remark-link-rewrite';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

// Base path for links to source files
const GITHUB_BLOB_URL = 'https://github.com/zherczeg/sljit/blob/';

// Revision to use when linking to source files.
// Defaults to master, but could be injected via an environment variable to
// support versioned docs
const REVISION = process.env.SLJIT_DOCS_SOURCE_REVISION
    ? process.env.SLJIT_DOCS_SOURCE_REVISION
    : 'master';

const config = {
  title: 'SLJIT',
  tagline: 'Platform-independent low-level JIT compiler',

  url: 'https://zherczeg.github.io',
  baseUrl: '/sljit/',

  organizationName: 'zherczeg',
  projectName: 'sljit',

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'throw',

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      ({
        docs: {
          path: '..', // '.' would be the website folder itself
          include: [
            'general/**/*.{md,mdx}',
            'tutorial/**/*.{md,mdx}'
          ],
          sidebarPath: './sidebars.js',
          editUrl: 'https://github.com/zherczeg/sljit/docs/docs/',
          // Some Markdown fixups to display nicely both on GitHub and via Docusaurus
          beforeDefaultRemarkPlugins: [
            // 1. Convert GitHub-style admonitions to Docusaurus directives
            remarkGithubAdmonitionsToDirectives,
            // 2. Convert relative links to source files to absolute links to the files on GitHub
            [
              RemarkLinkRewrite, {
                replacer: (url) => {
                  if (url.startsWith('sources/')) {
                    return GITHUB_BLOB_URL + REVISION + '/docs/tutorial/' + url;
                  }
                  return url
                }
              }
            ]
          ]
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      }),
    ],
  ],

  themeConfig:
    ({
      navbar: {
        title: 'SLJIT',
        items: [
          {
            type: 'docSidebar',
            sidebarId: 'generalSidebar',
            position: 'left',
            label: 'Docs',
          },
          {
            type: 'docSidebar',
            sidebarId: 'tutorialSidebar',
            position: 'left',
            label: 'Tutorial',
          },
          {
            'aria-label': 'GitHub',
            className: 'navbar--github-link',
            href: 'https://github.com/zherczeg/sljit',
            position: 'right',
          },
        ],
      },
      docs: {
        sidebar: {
          hideable: true,
        },
      },
      footer: {
        style: 'light',
        links: [
          {
            title: 'Content',
            items: [
              {
                label: 'Docs',
                to: '/docs/general/introduction',
              },
              {
                label: 'Tutorial',
                to: '/docs/tutorial/overview',
              },
            ],
          },
          {
            title: 'More',
            items: [
              {
                label: 'GitHub',
                href: 'https://github.com/zherczeg/sljit',
              },
            ],
          },
          {
            title: 'Attribution',
            items: [
              {
                html: `
                  <span>
                    <a href="https://www.figma.com/community/file/1166831539721848736/solar-icons-set">
                      Solar Icon Set
                    </a>
                    by
                    <a href="https://www.figma.com/@480design">
                      480 Design
                    </a>
                    and
                    <a href="https://www.figma.com/@voidrainbow">
                      R4IN80W
                    </a>
                    |
                    <a href="http://creativecommons.org/licenses/by/4.0/">
                      CC BY 4.0
                    </a>
                  </span>
                `
              }
            ]
          }
        ],
        copyright: `Copyright © ${new Date().getFullYear()} SLJIT contributors. Built with Docusaurus.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
      },
    }),
};

export default config;
