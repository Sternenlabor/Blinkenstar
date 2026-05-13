import { readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import fontkit from '@pdf-lib/fontkit';
import { PDFDocument, StandardFonts, rgb } from 'pdf-lib';

const fencedDivOpenPattern = /^:::\s*\{([^}]*)\}\s*$/;
const fencedDivClosePattern = /^:::\s*$/;
const markdownImagePattern = /^!\[([^\]]*)\]\(([^)]+)\)\s*\{([^}]*)\}\s*$/;
const readableMarkdownImagePattern = /^!\[([^\]]*)\]\((\S+)(?:\s+"([^"]*)")?\)\s*$/;
const referenceDefinitionPattern = /^\[([^\]]+)\]:\s*<>\s*"([^"]*)"\s*$/;
const pageHeadingPattern = /^##\s+Page\s+(\d+)\s*$/;
const fallbackFonts = new Map();
const reflowGapMultiplier = 1.6;
const bodyTextRightMargin = 32;
const bodyTextColor = '#33353b';
const bodyTextLineHeight = 13.8;
const pageBottomMargin = 20;
const maxBodyTextGap = 12;
const imageTextGap = 16;

export function parsePdfPages(markdown) {
  const lines = markdown.split(/\r?\n/);
  const readablePages = parseReadableMarkdownPages(lines);

  if (readablePages.length > 0) {
    for (const page of readablePages) {
      validatePage(page);
    }

    return readablePages;
  }

  const pages = parseFencedMarkdownPages(lines);

  if (pages.length === 0) {
    throw new Error('No readable Markdown page definitions found.');
  }

  return pages;
}

function parseFencedMarkdownPages(lines) {
  const pages = [];

  for (let index = 0; index < lines.length; index += 1) {
    const fence = parseFencedDivOpen(lines[index]);

    if (!fence?.classes.includes('pdf-page')) {
      continue;
    }

    const block = readFencedBlock(lines, index);
    const page = parseMarkdownPage(fence, block.lines);
    validatePage(page);
    pages.push(page);
    index = block.endIndex;
  }

  return pages;
}

export async function createPdfFromMarkdown(markdownPath, outputPath) {
  const markdown = await readFile(markdownPath, 'utf8');
  const pages = parsePdfPages(markdown);
  const pdf = await PDFDocument.create();
  pdf.registerFontkit(fontkit);

  const markdownDir = path.dirname(markdownPath);
  const fontCache = new Map();
  const imageCache = new Map();

  for (const pageDefinition of pages) {
    const page = pdf.addPage([pageDefinition.width, pageDefinition.height]);

    if (pageDefinition.background) {
      page.drawRectangle({
        x: 0,
        y: 0,
        width: pageDefinition.width,
        height: pageDefinition.height,
        color: parseColor(pageDefinition.background),
      });
    }

    for (const element of pageDefinition.elements) {
      if (element.type === 'image') {
        const image = await loadImage(pdf, imageCache, markdownDir, element.src);
        page.drawImage(image, {
          x: element.x,
          y: pageDefinition.height - element.y - element.height,
          width: element.width,
          height: element.height,
          opacity: element.opacity ?? 1,
        });
        continue;
      }

      if (element.type === 'text') {
        const font = await loadFont(pdf, fontCache, markdownDir, element.font);
        const size = element.size;
        const lineHeight = lineHeightForElement(element);
        const lines = wrapTextLines(String(element.text), font, size, element.width);

        for (let lineIndex = 0; lineIndex < lines.length; lineIndex += 1) {
          page.drawText(lines[lineIndex], {
            x: element.x,
            y: pageDefinition.height - element.y - size - lineIndex * lineHeight,
            size,
            font,
            color: parseColor(element.color ?? '#000000'),
            opacity: element.opacity ?? 1,
          });
        }
        continue;
      }

      if (element.type === 'rect') {
        page.drawRectangle({
          x: element.x,
          y: pageDefinition.height - element.y - element.height,
          width: element.width,
          height: element.height,
          color: parseColor(element.color ?? '#ffffff'),
          opacity: element.opacity ?? 1,
        });
      }
    }
  }

  await writeFile(outputPath, await pdf.save());
}

function parseReadableMarkdownPages(lines) {
  const pages = [];

  for (let index = 0; index < lines.length; index += 1) {
    const pageReference = parseReferenceDefinition(lines[index]);

    if (!pageReference?.id.startsWith('page-')) {
      continue;
    }

    const pageNumber = readIntegerAttribute(pageReference.attrs, 'page');

    let endIndex = lines.length;
    for (let nextIndex = index + 1; nextIndex < lines.length; nextIndex += 1) {
      const nextReference = parseReferenceDefinition(lines[nextIndex]);

      if (nextReference?.id.startsWith('page-')) {
        endIndex = nextIndex;
        break;
      }
    }

    const page = parseReadableMarkdownPage(pageNumber, pageReference.attrs, lines.slice(index + 1, endIndex));
    pages.push(fitPageContent(reflowReadableText(page, { compactGaps: true })));
    index = endIndex - 1;
  }

  return pages;
}

function reflowReadableText(page, { compactGaps = false } = {}) {
  const flowElements = page.elements
    .filter((element) => isFlowElement(element))
    .sort((left, right) => left.y - right.y || left.x - right.x);

  let previous = null;

  for (const element of flowElements) {
    if (!previous) {
      previous = element;
      continue;
    }

    if (!shouldReflowAfter(previous, element, compactGaps)) {
      previous = element;
      continue;
    }

    const expectedY = previous.y + elementBlockHeight(previous) + textGap(previous, element);

    if (Math.abs(element.y - expectedY) > 0.001) {
      shiftElementsAtOrAfter(page, element.y - markerLeadInGap(element), expectedY - element.y);
    }

    previous = element;
  }

  return page;
}

function fitPageContent(page) {
  for (const element of page.elements) {
    if (element.type !== 'image') {
      continue;
    }

    const maxBottom = page.height - pageBottomMargin;
    const overflow = element.y + element.height - maxBottom;

    if (overflow <= 0) {
      continue;
    }

    const availableHeight = maxBottom - element.y;

    if (availableHeight > 0) {
      const scale = Math.min(1, availableHeight / element.height);
      const originalWidth = element.width;
      const originalHeight = element.height;
      element.width = Number((element.width * scale).toFixed(3));
      element.height = Number((element.height * scale).toFixed(3));

      if (element.y + element.height > maxBottom) {
        element.height = Number(availableHeight.toFixed(3));
        element.width = Number((originalWidth * (element.height / originalHeight)).toFixed(3));
      }
      continue;
    }

    element.y = Math.max(0, Number((maxBottom - element.height).toFixed(3)));
  }

  return page;
}

function isFlowText(element) {
  return element.width === undefined || element.width >= 20;
}

function isFlowElement(element) {
  if (element.type === 'text') {
    return isFlowText(element);
  }

  return element.type === 'image' || element.type === 'rect';
}

function shouldReflowAfter(previous, element, compactGaps) {
  if (element.y <= previous.y) {
    return false;
  }

  if (!isBodyTextElement(previous)) {
    return false;
  }

  const gap = element.y - (previous.y + elementBlockHeight(previous));
  const expectedGap = textGap(previous, element);

  if (!horizontallyRelated(previous, element)) {
    return false;
  }

  return gap < expectedGap || (compactGaps && gap > maxAllowedGap(previous, element));
}

function maxAllowedGap(previous, element) {
  if (isBodyTextElement(previous) && isBodyTextElement(element)) {
    return maxBodyTextGap;
  }

  return textGap(previous, element) * reflowGapMultiplier;
}

function horizontallyRelated(previous, element) {
  const previousLeft = previous.x;
  const previousRight = previous.x + (previous.width ?? 0);
  const elementLeft = element.x;
  const elementRight = element.x + (element.width ?? 0);

  if (Math.abs(previousLeft - elementLeft) <= 8) {
    return true;
  }

  if (previous.width === undefined || element.width === undefined) {
    return false;
  }

  return Math.min(previousRight, elementRight) - Math.max(previousLeft, elementLeft) > 0;
}

function shiftElementsAtOrAfter(page, y, offset) {
  for (const element of page.elements) {
    if (element.y >= y) {
      element.y += offset;
    }
  }
}

function elementBlockHeight(element) {
  if (element.type === 'image' || element.type === 'rect') {
    return element.height;
  }

  return estimateTextLineCount(element) * lineHeightForElement(element);
}

function estimateTextLineCount(element) {
  const paragraphs = element.text ? String(element.text).split(/\n\s*\n/) : [''];

  if (!isBodyTextElement(element) || !element.width) {
    return paragraphs.reduce((count, paragraph) => count + Math.max(1, paragraph.split(/\r?\n/).length), 0);
  }

  const averageCharacterWidth = element.size * 0.42;
  const charactersPerLine = Math.max(1, Math.floor(element.width / averageCharacterWidth));

  return paragraphs.reduce((count, paragraph) => {
    const normalized = paragraph.replace(/\s+/g, ' ').trim();
    return count + Math.max(1, Math.ceil(normalized.length / charactersPerLine));
  }, 0);
}

function textGap(previous, element) {
  if (isBodyTextElement(previous) && element.type === 'text' && element.size < 9) {
    return 28;
  }

  if (isBodyTextElement(previous) && isBodyTextElement(element)) {
    return maxBodyTextGap;
  }

  if (previous.type === 'image' || element.type === 'image') {
    return imageTextGap;
  }

  return Math.max(previous.size ?? 0, element.size ?? 0);
}

function markerLeadInGap(element) {
  return Math.max(element.size ?? 0, element.lineHeight ?? 0, 20);
}

function normalizeTextElement(page, element) {
  if (isHeadingTextElement(element)) {
    return {
      ...element,
      width: Number((page.width - element.x - bodyTextRightMargin).toFixed(3)),
    };
  }

  if (!isBodyTextElement(element)) {
    return element;
  }

  const maxWidth = Number((page.width - element.x - bodyTextRightMargin).toFixed(3));

  return {
    ...element,
    text: normalizeBodyText(element.text),
    width: element.width === undefined ? maxWidth : Math.min(element.width, maxWidth),
    color: bodyTextColor,
    lineHeight: bodyTextLineHeight,
  };
}

function isHeadingTextElement(element) {
  return element.type === 'text'
    && element.font?.includes('Bold')
    && element.size >= 14
    && String(element.text).trim().length > 1;
}

function isBodyTextElement(element) {
  return element.type === 'text'
    && element.font === 'Calibri'
    && element.size === 9
    && (element.width ?? 0) >= 100;
}

function lineHeightForElement(element) {
  if (element.lineHeight !== undefined) {
    return element.lineHeight;
  }

  return isBodyTextElement(element) ? bodyTextLineHeight : element.size * 1.2;
}

function normalizeBodyText(text) {
  return String(text)
    .split(/\n\s*\n/)
    .map((paragraph) => paragraph.replace(/\s*\n\s*/g, ' ').replace(/\s+/g, ' ').trim())
    .filter(Boolean)
    .join('\n\n');
}

function wrapTextLines(text, font, size, width) {
  const lines = String(text).split(/\r?\n/);

  if (!width) {
    return lines;
  }

  return lines.flatMap((line) => wrapTextLine(line, font, size, width));
}

function wrapTextLine(line, font, size, width) {
  if (!line.trim()) {
    return [''];
  }

  const words = line.trim().split(/\s+/);
  const wrapped = [];
  let current = '';

  for (const word of words) {
    const candidate = current ? `${current} ${word}` : word;

    if (!current || font.widthOfTextAtSize(candidate, size) <= width) {
      current = candidate;
      continue;
    }

    wrapped.push(current);
    current = word;
  }

  if (current) {
    wrapped.push(current);
  }

  return wrapped;
}

function parseReadableMarkdownPage(pageNumber, pageReference, lines) {
  const page = {
    page: readIntegerAttribute(pageReference, 'page'),
    width: readNumberAttribute(pageReference, 'width'),
    height: readNumberAttribute(pageReference, 'height'),
    background: pageReference.attrs.background,
    elements: [],
  };

  if (page.page !== pageNumber) {
    throw new Error(`Page heading ${pageNumber} does not match page reference ${page.page}.`);
  }

  let pendingTextReference = null;

  for (let index = 0; index < lines.length; index += 1) {
    const line = lines[index].trim();

    if (!line) {
      continue;
    }

    const reference = parseReferenceDefinition(line);
    if (reference) {
      pendingTextReference = reference.id.startsWith('text-') ? reference : null;
      continue;
    }

    const image = parseReadableMarkdownImage(line);
    if (image) {
      page.elements.push(omitUndefined({
        type: 'image',
        id: image.alt || undefined,
        src: image.src,
        x: readNumberAttribute(image.attrs, 'x'),
        y: readNumberAttribute(image.attrs, 'y'),
        width: readNumberAttribute(image.attrs, 'width'),
        height: readNumberAttribute(image.attrs, 'height'),
        opacity: readOptionalNumberAttribute(image.attrs, 'opacity'),
      }));
      continue;
    }

    if (pendingTextReference) {
      const textBlock = readReadableTextBlock(lines, index);
      page.elements.push(normalizeTextElement(page, omitUndefined({
        type: 'text',
        id: pendingTextReference.id,
        text: textBlock.text,
        x: readNumberAttribute(pendingTextReference.attrs, 'x'),
        y: readNumberAttribute(pendingTextReference.attrs, 'y'),
        width: readOptionalNumberAttribute(pendingTextReference.attrs, 'width'),
        font: pendingTextReference.attrs.attrs.font ?? 'Helvetica',
        size: readNumberAttribute(pendingTextReference.attrs, 'size'),
        color: pendingTextReference.attrs.attrs.color ?? '#000000',
        lineHeight: readOptionalNumberAttribute(pendingTextReference.attrs, 'line-height'),
        opacity: readOptionalNumberAttribute(pendingTextReference.attrs, 'opacity'),
      })));
      pendingTextReference = null;
      index = textBlock.endIndex;
    }
  }

  return page;
}

/**
 * Reads readable Markdown text until the next layout marker while preserving paragraph breaks.
 */
function readReadableTextBlock(lines, startIndex) {
  const blockLines = [];
  let endIndex = startIndex;

  for (let index = startIndex; index < lines.length; index += 1) {
    const line = lines[index];
    const trimmed = line.trim();

    if (index !== startIndex && (parseReferenceDefinition(trimmed) || parseReadableMarkdownImage(trimmed) || pageHeadingPattern.test(trimmed))) {
      endIndex = index - 1;
      break;
    }

    blockLines.push(stripReadableMarkdownText(trimmed));
    endIndex = index;
  }

  while (blockLines.at(-1) === '') {
    blockLines.pop();
  }

  return {
    text: blockLines.join('\n').trim(),
    endIndex,
  };
}

function stripReadableMarkdownText(line) {
  return line
    .replace(/^#{1,6}\s+/, '')
    .replace(/^\*\*(.*)\*\*$/, '$1')
    .trim();
}

function parseReferenceDefinition(line) {
  const match = referenceDefinitionPattern.exec(line.trim());

  if (!match) {
    return null;
  }

  return {
    id: match[1],
    attrs: parseAttributes(match[2]),
  };
}

function parseReadableMarkdownImage(line) {
  const match = readableMarkdownImagePattern.exec(line.trim());

  if (!match) {
    return null;
  }

  return {
    alt: match[1],
    src: match[2],
    attrs: parseAttributes(match[3] ?? ''),
  };
}

function parseMarkdownPage(fence, lines) {
  const page = {
    page: readIntegerAttribute(fence, 'page'),
    width: readNumberAttribute(fence, 'width'),
    height: readNumberAttribute(fence, 'height'),
    background: fence.attrs.background,
    elements: [],
  };

  for (let index = 0; index < lines.length; index += 1) {
    const line = lines[index].trim();
    const image = parseMarkdownImage(line);

    if (image?.attrs.classes.includes('pdf-image')) {
      page.elements.push(omitUndefined({
        type: 'image',
        id: image.attrs.id,
        src: image.src,
        x: readNumberAttribute(image.attrs, 'x'),
        y: readNumberAttribute(image.attrs, 'y'),
        width: readNumberAttribute(image.attrs, 'width'),
        height: readNumberAttribute(image.attrs, 'height'),
        opacity: readOptionalNumberAttribute(image.attrs, 'opacity'),
      }));
      continue;
    }

    const childFence = parseFencedDivOpen(line);

    if (childFence?.classes.includes('pdf-text')) {
      const block = readFencedBlock(lines, index);
      page.elements.push(omitUndefined({
        type: 'text',
        id: childFence.id,
        text: block.lines.join('\n').trim(),
        x: readNumberAttribute(childFence, 'x'),
        y: readNumberAttribute(childFence, 'y'),
        width: readOptionalNumberAttribute(childFence, 'width'),
        font: childFence.attrs.font ?? 'Helvetica',
        size: readNumberAttribute(childFence, 'size'),
        color: childFence.attrs.color ?? '#000000',
        lineHeight: readOptionalNumberAttribute(childFence, 'line-height'),
        opacity: readOptionalNumberAttribute(childFence, 'opacity'),
      }));
      index = block.endIndex;
      continue;
    }

    if (childFence?.classes.includes('pdf-rect')) {
      const block = readFencedBlock(lines, index);
      page.elements.push(omitUndefined({
        type: 'rect',
        id: childFence.id,
        x: readNumberAttribute(childFence, 'x'),
        y: readNumberAttribute(childFence, 'y'),
        width: readNumberAttribute(childFence, 'width'),
        height: readNumberAttribute(childFence, 'height'),
        color: childFence.attrs.color ?? '#ffffff',
        opacity: readOptionalNumberAttribute(childFence, 'opacity'),
      }));
      index = block.endIndex;
    }
  }

  return page;
}

function parseFencedDivOpen(line) {
  const match = fencedDivOpenPattern.exec(line.trim());

  if (!match) {
    return null;
  }

  return parseAttributes(match[1]);
}

function readFencedBlock(lines, startIndex) {
  let depth = 1;
  const blockLines = [];

  for (let index = startIndex + 1; index < lines.length; index += 1) {
    const line = lines[index];

    if (parseFencedDivOpen(line)) {
      depth += 1;
      blockLines.push(line);
      continue;
    }

    if (fencedDivClosePattern.test(line.trim())) {
      depth -= 1;

      if (depth === 0) {
        return { lines: blockLines, endIndex: index };
      }

      blockLines.push(line);
      continue;
    }

    blockLines.push(line);
  }

  throw new Error(`Unclosed fenced div starting at line ${startIndex + 1}.`);
}

function parseMarkdownImage(line) {
  const match = markdownImagePattern.exec(line);

  if (!match) {
    return null;
  }

  return {
    alt: match[1],
    src: match[2],
    attrs: parseAttributes(match[3]),
  };
}

function parseAttributes(source) {
  const result = {
    id: undefined,
    classes: [],
    attrs: {},
  };
  let index = 0;

  while (index < source.length) {
    while (source[index] === ' ' || source[index] === '\t') index += 1;
    if (index >= source.length) break;

    if (source[index] === '#') {
      const token = readBareToken(source, index + 1);
      result.id = token.value;
      index = token.endIndex;
      continue;
    }

    if (source[index] === '.') {
      const token = readBareToken(source, index + 1);
      result.classes.push(token.value);
      index = token.endIndex;
      continue;
    }

    const key = readKey(source, index);
    index = key.endIndex;
    while (source[index] === ' ' || source[index] === '\t') index += 1;

    if (source[index] !== '=') {
      result.classes.push(key.value);
      continue;
    }

    index += 1;
    const value = readAttributeValue(source, index);
    result.attrs[key.value] = value.value;
    index = value.endIndex;
  }

  return result;
}

function readBareToken(source, startIndex) {
  let index = startIndex;

  while (index < source.length && !/\s/.test(source[index])) {
    index += 1;
  }

  return { value: source.slice(startIndex, index), endIndex: index };
}

function readKey(source, startIndex) {
  let index = startIndex;

  while (index < source.length && !/[\s=]/.test(source[index])) {
    index += 1;
  }

  return { value: source.slice(startIndex, index), endIndex: index };
}

function readAttributeValue(source, startIndex) {
  if (source[startIndex] === '"' || source[startIndex] === "'") {
    const quote = source[startIndex];
    let index = startIndex + 1;

    while (index < source.length && source[index] !== quote) {
      index += 1;
    }

    return { value: source.slice(startIndex + 1, index), endIndex: index + 1 };
  }

  return readBareToken(source, startIndex);
}

function readIntegerAttribute(attributeSet, key) {
  const value = Number(readRequiredAttribute(attributeSet, key));

  if (!Number.isInteger(value)) {
    throw new Error(`Attribute ${key} must be an integer.`);
  }

  return value;
}

function readNumberAttribute(attributeSet, key) {
  const value = Number(readRequiredAttribute(attributeSet, key));

  if (!Number.isFinite(value)) {
    throw new Error(`Attribute ${key} must be a number.`);
  }

  return value;
}

function readOptionalNumberAttribute(attributeSet, key) {
  if (attributeSet.attrs[key] === undefined) {
    return undefined;
  }

  const value = Number(attributeSet.attrs[key]);

  if (!Number.isFinite(value)) {
    throw new Error(`Attribute ${key} must be a number.`);
  }

  return value;
}

function readRequiredAttribute(attributeSet, key) {
  const value = attributeSet.attrs[key];

  if (value === undefined || value === '') {
    throw new Error(`Missing required attribute ${key}.`);
  }

  return value;
}

function omitUndefined(object) {
  return Object.fromEntries(Object.entries(object).filter(([, value]) => value !== undefined));
}

function validatePage(page) {
  assertPositiveInteger(page.page, 'page');
  assertPositiveNumber(page.width, `page ${page.page} width`);
  assertPositiveNumber(page.height, `page ${page.page} height`);

  if (!Array.isArray(page.elements)) {
    throw new Error(`Page ${page.page} must have an elements array.`);
  }

  for (const element of page.elements) {
    validateElement(page.page, element);
  }
}

function validateElement(pageNumber, element) {
  if (!element || typeof element !== 'object') {
    throw new Error(`Page ${pageNumber} contains an invalid element.`);
  }

  if (!['image', 'text', 'rect'].includes(element.type)) {
    throw new Error(`Page ${pageNumber} contains unsupported element type: ${element.type}`);
  }

  assertPositiveNumberOrZero(element.x, `page ${pageNumber} ${element.id ?? element.type} x`);
  assertPositiveNumberOrZero(element.y, `page ${pageNumber} ${element.id ?? element.type} y`);

  if (element.type === 'image') {
    if (!element.src) {
      throw new Error(`Page ${pageNumber} image element ${element.id ?? '<missing id>'} is missing src.`);
    }
    assertPositiveNumber(element.width, `page ${pageNumber} ${element.id ?? 'image'} width`);
    assertPositiveNumber(element.height, `page ${pageNumber} ${element.id ?? 'image'} height`);
  }

  if (element.type === 'text') {
    if (typeof element.text !== 'string') {
      throw new Error(`Page ${pageNumber} text element ${element.id ?? '<missing id>'} must have text.`);
    }
    assertPositiveNumber(element.size, `page ${pageNumber} ${element.id ?? 'text'} size`);
  }

  if (element.type === 'rect') {
    assertPositiveNumber(element.width, `page ${pageNumber} ${element.id ?? 'rect'} width`);
    assertPositiveNumber(element.height, `page ${pageNumber} ${element.id ?? 'rect'} height`);
  }
}

function assertPositiveInteger(value, label) {
  if (!Number.isInteger(value) || value < 1) {
    throw new Error(`${label} must be a positive integer.`);
  }
}

function assertPositiveNumber(value, label) {
  if (!Number.isFinite(value) || value <= 0) {
    throw new Error(`${label} must be a positive number.`);
  }
}

function assertPositiveNumberOrZero(value, label) {
  if (!Number.isFinite(value) || value < 0) {
    throw new Error(`${label} must be zero or a positive number.`);
  }
}

async function loadImage(pdf, cache, markdownDir, imageSource) {
  const imagePath = path.resolve(markdownDir, imageSource);

  if (cache.has(imagePath)) {
    return cache.get(imagePath);
  }

  const imageBytes = await readFile(imagePath);
  const extension = path.extname(imagePath).toLowerCase();
  let image;

  if (extension === '.png') {
    image = await pdf.embedPng(imageBytes);
  } else if (extension === '.jpg' || extension === '.jpeg') {
    image = await pdf.embedJpg(imageBytes);
  } else {
    throw new Error(`Unsupported image type for ${imagePath}. Use PNG or JPEG.`);
  }

  cache.set(imagePath, image);
  return image;
}

async function loadFont(pdf, cache, markdownDir, fontName = 'Helvetica') {
  if (cache.has(fontName)) {
    return cache.get(fontName);
  }

  const fontPath = resolveFontPath(markdownDir, fontName);
  let font;

  if (fontPath) {
    font = await pdf.embedFont(await readFile(fontPath), { subset: true });
  } else {
    font = await pdf.embedFont(resolveStandardFont(fontName));
  }

  cache.set(fontName, font);
  return font;
}

function resolveFontPath(markdownDir, fontName) {
  if (fontName.includes('/') || fontName.includes('\\')) {
    return path.resolve(markdownDir, fontName);
  }

  const mappedPath = {
    Calibri: 'node_modules/@fontsource/carlito/files/carlito-latin-400-normal.woff',
    'Calibri-Bold': 'node_modules/@fontsource/carlito/files/carlito-latin-700-normal.woff',
    Carlito: 'node_modules/@fontsource/carlito/files/carlito-latin-400-normal.woff',
    'Carlito-Bold': 'node_modules/@fontsource/carlito/files/carlito-latin-700-normal.woff',
  }[fontName];

  return mappedPath ? path.resolve(markdownDir, mappedPath) : null;
}

function resolveStandardFont(fontName) {
  if (fallbackFonts.has(fontName)) {
    return fallbackFonts.get(fontName);
  }

  const mappedFont = {
    Helvetica: StandardFonts.Helvetica,
    'Helvetica-Bold': StandardFonts.HelveticaBold,
    Times: StandardFonts.TimesRoman,
    'Times-Bold': StandardFonts.TimesRomanBold,
    Courier: StandardFonts.Courier,
    'Courier-Bold': StandardFonts.CourierBold,
  }[fontName] ?? StandardFonts.Helvetica;

  fallbackFonts.set(fontName, mappedFont);
  return mappedFont;
}

function parseColor(color) {
  const match = /^#?([0-9a-f]{6})$/i.exec(color);

  if (!match) {
    throw new Error(`Unsupported color "${color}". Use #rrggbb.`);
  }

  const value = match[1];
  return rgb(
    Number.parseInt(value.slice(0, 2), 16) / 255,
    Number.parseInt(value.slice(2, 4), 16) / 255,
    Number.parseInt(value.slice(4, 6), 16) / 255,
  );
}

async function main() {
  const [, , markdownPath = 'manual.md', outputPath = 'manual.generated.pdf'] = process.argv;
  await createPdfFromMarkdown(path.resolve(markdownPath), path.resolve(outputPath));
  console.log(`Generated ${outputPath}`);
}

const currentFile = fileURLToPath(import.meta.url);

if (process.argv[1] && path.resolve(process.argv[1]) === currentFile) {
  main().catch((error) => {
    console.error(error.message);
    process.exitCode = 1;
  });
}
