# Release Checklist

Before tagging a new release (e.g., `v1.0.0`), ensure the following conditions are met:

- [ ] **Build passes:** Project successfully builds locally in `Release` configuration without warnings.
- [ ] **Tests pass:** `ctest --test-dir build -C Release` runs all unit and integration tests successfully.
- [ ] **Benchmarks run:** `BenchmarkRunner` metrics are recorded to ensure no regressions in requests/sec, latency, or queue depth.
- [ ] **CI Pipeline is Green:** GitHub Actions pipeline passes on both Windows and Ubuntu.
- [ ] **Docker builds locally:** `docker-compose build` completes successfully.
- [ ] **README updated:** Performance metrics, architecture diagrams, and version tags are current.
- [ ] **Version Bump:** Config or CMake version matches the new release.
- [ ] **Tag created:** `git tag vX.Y.Z` and `git push origin vX.Y.Z`.
