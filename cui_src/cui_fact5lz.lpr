program cui_fact5lz;

{$MODE Delphi}

{$APPTYPE CONSOLE}

uses
  SysUtils,
  Classes;

const
  LZ77lib = 'FACT5LZ.dll';

type
  TBytesArray = array [0 .. $200000 - 1] of byte;
  PBytesArray = ^TBytesArray;

  // compress(unsigned char *input, unsigned char *output, int size);
  // decompress(unsigned char *input, unsigned char *output);
function Compress(READBUFFER, WRITEBUFFER: Pointer; SIZE: Integer): Integer; cdecl;
  external LZ77lib name 'compress';
function Decompress(READBUFFER, WRITEBUFFER: Pointer): Integer; cdecl;
  external LZ77lib name 'decompress';
function CompressedSize(READBUFFER: Pointer): Integer; cdecl;
 external LZ77lib name 'compressed_size';

function FileSize(const FileName: string): Int64;
var
  F: TFileStream;
begin
  F := TFileStream.Create(FileName, fmOpenRead);
  Result := F.SIZE;
  F.Free;
end;

function HexToInt(const Str: string): integer;
var
  i, r: integer;
begin
  val('$' + Trim(Str), r, i);
  if i <> 0 then
    Result := 0
  else
    Result := r;
end;

procedure CHC_Compress(const inStr: string);
var
  C_STREAM, D_STREAM: TFileStream;
  D_SIZE, C_SIZE: integer;
  outStr: string;
  D_BUF, C_BUF: PBytesArray;
  r: Cardinal;
begin
  D_STREAM := TFileStream.Create(inStr, fmOpenRead or fmShareExclusive);
  New(D_BUF);
  New(C_BUF);

  D_SIZE := D_STREAM.Read(D_BUF^[0], D_STREAM.Size);
  C_SIZE := Compress(D_BUF, C_BUF, D_SIZE);

  outStr := ChangeFileExt(inStr, Format('.cmp%s', [ExtractFileExt(inStr)]));

  C_STREAM := TFileStream.Create(outStr, fmOpenWrite or fmCreate or
    fmShareExclusive);
  C_STREAM.Write(C_BUF^[0], C_SIZE);
  C_STREAM.Free;

  r := C_SIZE shl 16 + D_SIZE;

  Writeln('Compressed file was save as: "' + ExtractFileName(outStr) + '";');
  Writeln('Decompressed size: ' + IntToStr(r and $FFFF) + ' bytes;');
  Writeln('Compressed size: ' + IntToStr(r and $FFFF0000 shr 16) + ' bytes.');

  Dispose(C_BUF);
  Dispose(D_BUF);
  D_STREAM.Free;
end;

procedure CHC_Decompress(const inStr: string; OFFSET: integer);
var
  C_STREAM, D_STREAM: TFileStream;
  C_BUF, D_BUF: PBytesArray;
  D_SIZE, C_SIZE: Integer;
  outStr: string;
  r: Cardinal;
begin
  C_STREAM := TFileStream.Create(inStr, fmOpenRead);

  outStr := ChangeFileExt(inStr, Format('.%.4X%s',
    [OFFSET, ExtractFileExt(inStr)]));
  D_STREAM := TFileStream.Create(outStr, fmOpenWrite or fmCreate or
    fmShareExclusive);

  New(C_BUF);
  New(D_BUF);

  C_STREAM.Read(C_BUF^[0], C_STREAM.Size);
  C_SIZE := CompressedSize(@C_BUF^[OFFSET]);
  D_SIZE := Decompress(@C_BUF^[OFFSET], D_BUF);

  D_STREAM.Write(D_BUF^[0], D_SIZE);
  r := C_SIZE shl 16 + D_SIZE;
  C_STREAM.Free;
  D_STREAM.Free;
  Dispose(C_BUF);
  Dispose(D_BUF);

  Writeln('Decompressed file was save as: "' + ExtractFileName(outStr) + '";');
  Writeln('Compressed size: ' + IntToStr(r and $FFFF0000 shr 16) + ' bytes');
  Writeln('Decompressed size: ' + IntToStr(r and $FFFF) + ' bytes.');
end;

procedure Help;
begin
  Writeln('-= (SMD) Factor5 LZ Tool v1.0 [by Lab 313] (08.03.2016) =-');
  Writeln('-----------------------------');
  Writeln('Compression type: LZ77');
  Writeln('De/Compressor: Dr. MefistO');
  Writeln('Coding: Dr. MefistO');
  Writeln('Our site: http://lab313.ru');
  Writeln('Info: This console tool allows you to compress and decompress' +
    #13#10 + '      data, packed with Factor5''s LZ77 algo.' + #13#10);
  Writeln('USAGE FOR DECOMPRESSION:' + #13#10 +
    'cui_fact5lz.exe [Filename] [HexOffset]' + #13#10 + 'EXAMPLE:' + #13#10 +
    'cui_fact5lz.exe ISSD.bin 14A488' + #13#10);
  Writeln('USAGE FOR COMPRESSION:' + #13#10 + 'cui_fact5lz.exe [InFilename]' +
    #13#10 + 'EXAMPLE:' + #13#10 + 'cui_fact5lz.exe ISSD.14A488.bin' + #13#10 +
    '-----------------------------' + #13#10);
end;

var
  OFFSET: integer;

{$R *.res}

begin
  Help;
  if not FileExists(ParamStr(1)) then
  begin
    Writeln('File not found: "' + ExtractFileName(ParamStr(1)) + '"' + #13#10);
    Exit;
  end;
  OFFSET := HexToInt(ParamStr(2));
  if FileSize(ParamStr(1)) <= OFFSET then
  begin
    Writeln('Specified offset is greater than size of packed data!' + #13#10);
    Exit;
  end;

  if ParamCount = 2 then
  begin
    Writeln('Decompressing "' + ExtractFileName(ParamStr(1)) + '" from 0x' +
      IntToHex(OFFSET, 4) + '...');
    CHC_Decompress(ParamStr(1), OFFSET);
    Exit;
  end
  else if ParamCount = 1 then
  begin
    Writeln('Compressing "' + ExtractFileName(ParamStr(1)) + '"...');
    CHC_Compress(ParamStr(1));
    Exit;
  end
  else
    Help;

  // Text2Clipboard(Int2Str(size));
end.
