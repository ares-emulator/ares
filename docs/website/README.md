# Website

This website is built using [Docusaurus](https://docusaurus.io/).

## Structure

- `src` contains the home page
- `static` contains static assets (images, ...); **NOTE:** the `.nojekyll` file is important, see [here](https://docusaurus.io/docs/deployment#docusaurusconfigjs-settings)
- `docusaurus.config.js` contains the Docusaurus related configuration
- `sidebar.js` contains the sidebar configuration

## Local Development

In order to preview changes to the website locally on your machine, you need the following:
- [Node.js](https://nodejs.org) >= 18.0
- npm (comes with Node.js on Windows / MacOS; separate package on Linux)

Install the dependencies:

```bash
cd docs/website
npm install
```

Above step needs to be done only once initially, or after dependencies have changed.

To start the development server, run the following:

```bash
npm run start
```

A browser window should open automatically (it may take some time to load initially as the website is processed). If it does not, take a look at the output of above command for more details.

## Adding Documentation

To add or change documentation, simply create or edit the corresponding Markdown file. No JavaScript or React knowledge needed.

Docusaurus has been configured to take the `general` and `tutorial` folders into account. If you want to add another section, create the corresponding folder structure and extend [`docusaurus.config.js`](./docusaurus.config.js) and [`sidebar.js`](./sidebars.js) accordingly.

## Changing the Homepage

The homepage is based on React and written in JavaScript. Its main entry point is the [`src/pages/index.js`](./src/pages/index.js) file.

## Upgrading Docusaurus

Docusaurus will let you know if a new version is available when you [start the development server](#local-development). See also [here](https://docusaurus.io/docs/migration).
