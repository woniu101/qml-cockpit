const sharp = require('sharp');
const path = require('path');
const fs = require('fs');
const root = path.resolve(__dirname, '..');
(async () => {
  for (const name of ['01-parking', '02-navigation', '03-vehicle-controls']) {
    const source = path.join(root, 'screens', name + '.svg');
    await sharp(source).png().toFile(path.join(root, 'screens', name + '.png'));
    await sharp(source, {density: 144}).png().toFile(path.join(root, 'screens', name + '@2x.png'));
    console.log(name, '1600x900 and 3200x1800');
  }
})();
