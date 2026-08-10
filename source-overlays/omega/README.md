# OMEGA source overlay archive

`Parallels-X-Omega-Source.zip` is the exact source overlay supplied for OMEGA Update 1 of 3.

- Original ZIP SHA-256: `e75925ace3a6a82fabc4e231677551075af5be7d7b75b457faf28937bfa5839a`
- Claimed baseline: `v0.4H.5-candidate`
- Baseline commit: `851b8613fac6451f62ef492ad0ee454622096f63`

The original ZIP is preserved byte-for-byte. It was not executable against its claimed baseline without three narrowly scoped corrections. Reproducible corrected copies are under `corrected/`:

1. The initial animation reset anchor occurred three times. The corrected patcher uses distinct start-chapter and load-snapshot contexts, leaving the existing enter-scene patch separate.
2. The Linux save-recovery blocks used ordinary Python triple-quoted strings, which converted C++ `\n` literals into real newlines and prevented anchor matching. The corrected patcher uses raw strings.
3. The replacement animation author emitted format version 2 even though the audited cooker supports the otherwise-identical version 1 schema. The corrected author emits version 1.

After those corrections, the strict check passed, the overlay applied, all five shared test suites passed, and the Mac and 3DS builds succeeded. The source changes live on the `agent/omega-builds` branch for review.

OMEGA deliberately does not include the later Graphics or Optimize updates and does not begin Chapter 2.
