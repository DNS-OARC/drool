local _, major, minor, patch = unpack(arg)
major = tonumber(major)
minor = tonumber(minor)
patch = tonumber(patch)
if DNSJIT_MAJOR_VERSION > major then os.exit(0) end
if DNSJIT_MAJOR_VERSION < major then os.exit(1) end
if DNSJIT_MINOR_VERSION > minor then os.exit(0) end
if DNSJIT_MINOR_VERSION < minor then os.exit(1) end
if DNSJIT_PATCH_VERSION > patch then os.exit(0) end
if DNSJIT_PATCH_VERSION < patch then os.exit(1) end
os.exit(0)
