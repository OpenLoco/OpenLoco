#r "System.Drawing.dll"

using System.Drawing;
using System.Drawing.Imaging;

if (Args.Count == 0)
{
    Console.WriteLine("Usage: makeico <directory>");
    return;
}

var inputDirectory = Args[0];
var outputPath = Path.Combine(inputDirectory, "icon.ico");
var imageSizes = new int[] { 256, 128, 96, 64, 48, 40, 32, 24, 16, 8, 4 };
var foundImages = imageSizes
    .Select(size => (size, Path.Combine(inputDirectory, "icon_x" + size + ".png")))
    .Where(x => File.Exists(x.Item2))
    .ToArray();

using (FileStream fs = new FileStream(outputPath, FileMode.Create))
{
    var bw = new BinaryWriter(fs);
    bw.Write((short)0);
    bw.Write((short)1);
    bw.Write((short)foundImages.Length);

    var dataStartOffset = 6 + (foundImages.Length * 16);

    using (var dataStream = new MemoryStream())
    {
        foreach (var (size, path) in foundImages)
        {
            bw.Write((byte)(size == 256 ? 0 : size));
            bw.Write((byte)(size == 256 ? 0 : size));
            bw.Write((byte)0);
            bw.Write((byte)0);
            bw.Write((short)0);
            bw.Write((short)32);

            int dataOffset = (int)dataStream.Position;
            int dataLength;

            Console.WriteLine("Importing {0}", Path.GetFileName(path));
            using (var image = Image.FromFile(path))
            {
                image.Save(dataStream, ImageFormat.Png);
            }

            dataLength = (int)dataStream.Position - dataOffset;
            dataOffset += dataStartOffset;

            bw.Write(dataLength);
            bw.Write(dataOffset);
        }
        bw.Write(dataStream.ToArray());
    }
}
