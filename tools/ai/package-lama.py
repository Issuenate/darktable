#!/usr/bin/env python3
"""Build a pinned LaMa package for preferences > ai models > install from file."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import urllib.request
import zipfile


REVISION = "c3c0c9e468934d62e79c329e35d82dd09ff8c444"
SOURCE = f"https://huggingface.co/Carve/LaMa-ONNX/resolve/{REVISION}/lama_fp32.onnx"
SHA256 = "1faef5301d78db7dda502fe59966957ec4b79dd64e16f03ed96913c7a4eb68d6"
MODEL_ID = "inpaint-lama-carve-512-v1"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="new .dtmodel archive to create")
    parser.add_argument("--model-file", type=Path, help="use an already downloaded lama_fp32.onnx")
    args = parser.parse_args()
    if args.output.suffix != ".dtmodel":
        parser.error("output must end in .dtmodel")
    if args.output.exists():
        parser.error("output already exists")

    with tempfile.TemporaryDirectory(prefix="darktable-lama-") as temporary:
        model = args.model_file
        if model is None:
            model = Path(temporary) / "model.onnx"
            print(f"Downloading {SOURCE}", flush=True)
            with urllib.request.urlopen(SOURCE, timeout=120) as source, model.open("wb") as dest:
                shutil.copyfileobj(source, dest)

        digest = hashlib.sha256()
        with model.open("rb") as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(chunk)
        if digest.hexdigest() != SHA256:
            parser.error("model checksum mismatch; no package was created")

        manifest = {
            "id": MODEL_ID,
            "name": "LaMa content-aware remove",
            "description": "Local object removal for retouch, using a 512 x 512 context crop",
            "task": "inpaint",
            "arch": "lama-carve-512",
            "backend": "onnx",
            "version": "1.0",
            "default": True,
            "num_inputs": 2,
            "author": "LaMa authors; ONNX export by Carve Photos",
            "source": SOURCE,
            "license": "Apache-2.0",
            "paper": "https://arxiv.org/abs/2109.07161",
            "notes": f"model.onnx sha256: {SHA256}; RGB float input [0,1], output [0,255]",
        }
        archive = Path(temporary) / "package.dtmodel"
        with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_STORED) as package:
            package.write(model, f"{MODEL_ID}/model.onnx")
            package.writestr(f"{MODEL_ID}/config.json", json.dumps(manifest, indent=2) + "\n")
        with archive.open("rb") as source, args.output.open("xb") as dest:
            shutil.copyfileobj(source, dest)
    print(f"Created {args.output}. Install it from preferences > ai models, then activate it.")


if __name__ == "__main__":
    main()
