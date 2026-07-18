{
  pkgs ? import <nixpkgs> { },
  lib ? pkgs.lib,
  quazip-src ? null,
}:

pkgs.stdenv.mkDerivation (finalAttrs: {
  pname = "nero-umu";
  version = "1.2.0";

  src = ./.;

  __structedAttrs = true;
  strictDeps = true;

  nativeBuildInputs = with pkgs; [
    cmake
    pkg-config
    qt6.qttools
    qt6.wrapQtAppsHook
  ];

  buildInputs = with pkgs; [
    zlib
    qt6.qtbase
    qt6.qt5compat
    qt6.qtsvg
    qt6.qtwayland
  ];

  postPatch = lib.optionalString (quazip-src != null) ''
    rm -rf lib/quazip
    mkdir -p lib
    cp -r ${quazip-src} lib/quazip
    chmod -R u+w lib/quazip
  '';

  qtWrapperArgs = [
    "--prefix PATH : ${lib.makeBinPath [ pkgs.umu-launcher ]}"
  ];

  env.LANG = "C.UTF-8";

  meta = {
    description = "Umu-launcher manager and launcher";
    homepage = "https://github.com/SeongGino/Nero-umu";
    license = lib.licenses.gpl3Only;
    platforms = lib.platforms.linux;
    mainProgram = "nero-umu";
  };
})
