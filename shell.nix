{ 
  pkgs ? import <nixpkgs> {}
}:
pkgs.mkShell {
  buildInputs = [
    pkgs.platformio
  ];
  shellHook = ''
  '';
}
