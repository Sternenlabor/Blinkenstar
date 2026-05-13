import assert from 'node:assert/strict';
import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import test from 'node:test';
import { PDFDocument } from 'pdf-lib';

import {
  createPdfFromMarkdown,
  parsePdfPages,
} from '../scripts/markdown-to-pdf.mjs';

const onePixelPng = Buffer.from(
  'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==',
  'base64',
);

test('parsePdfPages reads readable markdown with hidden layout references', () => {
  const markdown = `
# Manual

[page-1]: <> "page=1 width=306.14173 height=436.53543 background=#ffffff"

![image-001](assets/manual-elements/page-001-image-001.png "x=10 y=20 width=40 height=50")

[text-001]: <> "x=31.18111 y=43.1919 font=Calibri-Bold size=14 color=#111111"

### BLINKENSTAR
`;

  assert.deepEqual(parsePdfPages(markdown), [
    {
      page: 1,
      width: 306.14173,
      height: 436.53543,
      background: '#ffffff',
      elements: [
        {
          type: 'image',
          id: 'image-001',
          src: 'assets/manual-elements/page-001-image-001.png',
          x: 10,
          y: 20,
          width: 40,
          height: 50,
        },
        {
          type: 'text',
          id: 'text-001',
          text: 'BLINKENSTAR',
          x: 31.18111,
          y: 43.1919,
          width: 242.961,
          font: 'Calibri-Bold',
          size: 14,
          color: '#111111',
        },
      ],
    },
  ]);
});

test('parsePdfPages compacts readable text gaps left by removed elements', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff reflow=true"

[text-001]: <> "x=50 y=100 width=205 font=Calibri size=9 color=#000000 line-height=15"

First line
Second line

[text-002]: <> "x=50 y=160 width=205 font=Calibri size=9 color=#000000"

Continuation

[text-003]: <> "x=50 y=320 width=205 font=Calibri size=9 color=#000000"

Warning after removed legend
`;

  const page = parsePdfPages(markdown)[0];

  assert.equal(page.elements.find((element) => element.id === 'text-001').y, 100);
  assert.equal(page.elements.find((element) => element.id === 'text-002').y, 125.8);
  assert.equal(page.elements.find((element) => element.id === 'text-003').y, 151.6);
});

test('parsePdfPages normalizes readable body text formatting', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff"

[text-001]: <> "x=53.858 y=95.091 width=232.625 font=Calibri size=9 color=#34363d"

Body text

[text-002]: <> "x=35.63 y=86.407 width=9.108 font=Calibri-Bold size=18 color=#fe4219"

### 3
`;

  const page = parsePdfPages(markdown)[0];
  const body = page.elements.find((element) => element.id === 'text-001');
  const marker = page.elements.find((element) => element.id === 'text-002');

  assert.equal(body.width, 220.142);
  assert.equal(body.color, '#33353b');
  assert.equal(body.lineHeight, 13.8);
  assert.equal(marker.width, 9.108);
  assert.equal(marker.color, '#fe4219');
});

test('parsePdfPages joins extracted body lines into paragraphs', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff"

[text-001]: <> "x=53.858 y=95.091 width=205 font=Calibri size=9 color=#33353b"

Drehe die Platine auf die Seite, auf der sich noch keine
Bauteile
befinden.
`;

  const body = parsePdfPages(markdown)[0].elements.find((element) => element.id === 'text-001');

  assert.equal(
    body.text,
    'Drehe die Platine auf die Seite, auf der sich noch keine Bauteile befinden.',
  );
});

test('parsePdfPages moves later flow text down when earlier body text grows', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff"

[text-001]: <> "x=53.858 y=95 width=110 font=Calibri size=9 color=#33353b line-height=15"

This paragraph is long enough to wrap across several rendered lines and therefore needs to move following text down.

[text-002]: <> "x=37.376 y=140 width=5.85 font=Calibri-Bold size=18 color=#fe4219"

### !

[text-003]: <> "x=53.858 y=147 width=205 font=Calibri size=9 color=#33353b line-height=15"

Warning text
`;

  const page = parsePdfPages(markdown)[0];

  assert.equal(page.elements.find((element) => element.id === 'text-002').y, 155.2);
  assert.equal(page.elements.find((element) => element.id === 'text-003').y, 162.2);
});

test('parsePdfPages gives headings enough width to avoid extraction-box wrapping', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff"

[text-001]: <> "x=31.181 y=203.189 width=86.812 font=Calibri-Bold size=14 color=#fe4219"

### LÖTEN LERNEN
`;

  const heading = parsePdfPages(markdown)[0].elements.find((element) => element.id === 'text-001');

  assert.equal(heading.width, 242.819);
});

test('parsePdfPages moves images down when body text grows into them', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff"

[text-001]: <> "x=36 y=220 width=205 font=Calibri size=9 color=#33353b line-height=15"

This paragraph is long enough to wrap across several rendered lines and therefore needs to move an image that used to sit below the original extracted text.

![image-001](page.png "x=90 y=260 width=114 height=85")
`;

  const image = parsePdfPages(markdown)[0].elements.find((element) => element.id === 'image-001');

  assert.equal(image.y, 277.4);
});

test('parsePdfPages caps large gaps between body text blocks', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff"

[text-001]: <> "x=53.858 y=95 width=205 font=Calibri size=9 color=#33353b line-height=15"

First paragraph.

[text-002]: <> "x=53.858 y=220 width=205 font=Calibri size=9 color=#33353b line-height=15"

Second paragraph.
`;

  const second = parsePdfPages(markdown)[0].elements.find((element) => element.id === 'text-002');

  assert.equal(second.y, 120.8);
});

test('parsePdfPages scales overflowing images into the page', () => {
  const markdown = `
[page-1]: <> "page=1 width=306 height=436 background=#ffffff"

![image-001](page.png "x=100 y=390 width=100 height=80")
`;

  const image = parsePdfPages(markdown)[0].elements.find((element) => element.id === 'image-001');

  assert.equal(image.y + image.height <= 416, true);
  assert.equal(image.height, 26);
  assert.equal(image.width, 32.5);
});

test('createPdfFromMarkdown creates pages with the markdown dimensions', async () => {
  const workspace = await mkdtemp(path.join(tmpdir(), 'manual-md-'));

  try {
    await writeFile(path.join(workspace, 'page.png'), onePixelPng);
    const markdownPath = path.join(workspace, 'manual.md');
    const outputPath = path.join(workspace, 'manual.generated.pdf');

    await writeFile(
      markdownPath,
      `[page-1]: <> "page=1 width=120 height=180 background=#ffffff"

![image-001](page.png "x=10 y=15 width=20 height=25")

[text-001]: <> "x=12 y=40 font=Helvetica size=10 color=#000000"

Editable text

[page-2]: <> "page=2 width=90 height=60"

![image-001](page.png "x=0 y=0 width=90 height=60")
`,
    );

    await createPdfFromMarkdown(markdownPath, outputPath);

    const generated = await PDFDocument.load(await readFile(outputPath));
    assert.equal(generated.getPageCount(), 2);
    assert.deepEqual(generated.getPage(0).getSize(), { width: 120, height: 180 });
    assert.deepEqual(generated.getPage(1).getSize(), { width: 90, height: 60 });
  } finally {
    await rm(workspace, { recursive: true, force: true });
  }
});
