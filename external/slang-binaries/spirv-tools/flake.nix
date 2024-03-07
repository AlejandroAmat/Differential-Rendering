{
  description = "A cross or static build of spirv-tools";

  inputs.nixpkgs.url =
    "github:NixOS/nixpkgs/88c052545610b9ee3e9b25144d855e897ce7728b";

  outputs = { self, nixpkgs }:
    let
      nativeSystems = [ "i686-linux" "x86_64-linux" "aarch64-linux" ];
      crossSystems = [ "mingw32" "mingwW64" ];
      forall = nixpkgs.lib.attrsets.genAttrs;
    in {
      packages = forall nativeSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system};
        in {
          static = pkgs.pkgsStatic.spirv-tools;
          cross = forall crossSystems (crossSystem:
            with pkgs.pkgsCross.${crossSystem};
            spirv-tools.overrideAttrs (old: {
              # Bundle this necessary dll with the binaries
              postInstall = "cp ${windows.mcfgthreads_pre_gcc_13}/bin/* $out/bin";
            })
          );
        });
    };
}
