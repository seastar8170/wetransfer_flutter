import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

DynamicLibrary _lib = Platform.isAndroid
    ? DynamicLibrary.open('libmy_functions.so')
    : DynamicLibrary.process();
final _getOpenCVVersion = _lib
    .lookup<NativeFunction<Pointer<Utf8> Function()>>('getOpenCVVersion')
    .asFunction<Pointer<Utf8> Function()>();
final _convertImageToGrayImage = _lib
    .lookup<
        NativeFunction<
            Void Function(Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>,
                Pointer<Utf8>)>>('convertImageToGrayImage')
    .asFunction<
        void Function(
            Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>)>();
final _water_shed = _lib
    .lookup<
        NativeFunction<
            Void Function(
                Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>)>>('water_shed')
    .asFunction<void Function(Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>)>();

String getOpenCVVersion() {
  return _getOpenCVVersion().cast<Utf8>().toDartString();
}

void convertImageToGrayImage(
    String inputPath, String outputPath, String tappoint, String colorhex) {
  _convertImageToGrayImage(inputPath.toNativeUtf8(), outputPath.toNativeUtf8(),
      tappoint.toNativeUtf8(), colorhex.toNativeUtf8());
}

void water_shed(String inputPath, String outputPath, String pointlist) {
  _water_shed(inputPath.toNativeUtf8(), outputPath.toNativeUtf8(),
      pointlist.toNativeUtf8());
}
