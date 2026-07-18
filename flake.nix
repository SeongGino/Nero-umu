{
  description = "Nero-UMU — an umu-launcher manager and launcher";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    quazip-src = {
      url = "github:stachenov/quazip";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      quazip-src,
    }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (pkgs: rec {
        nero-umu = pkgs.callPackage ./default.nix { inherit quazip-src; };
        default = nero-umu;
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.nero-umu ];
          packages = with pkgs; [
            gdb
            ninja
          ];
        };
      });

      formatter = forAllSystems (pkgs: pkgs.nixfmt-rfc-style);
    };
}
