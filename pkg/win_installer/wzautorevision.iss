// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025 Warzone 2100 Project
//
// Helpers for extracting autorevision components from the resource autorevision string
// (stored in "Comments" - see: platforms/windows/autorevision_rc.cmake)
//

[Code]

#define ConsumeToFirstColon(str *S) \
  Local[0] = Copy(S, 1, (Local[1] = Pos(":", S)) - 1), \
  S = Copy(S, Local[1] + 1), \
  Local[0]

#define GetAutoRevisionComponents(str S, *Tag, *Branch, *Commit, *VcsExtra) \
  Local[0] = S, \
  Local[0] == "" ? "" : (ConsumeToFirstColon(Local[0]), \
  Tag      = ConsumeToFirstColon(Local[0]), \
  Branch   = ConsumeToFirstColon(Local[0]), \
  Commit   = ConsumeToFirstColon(Local[0]), \
  VcsExtra = ConsumeToFirstColon(Local[0]), \
  "")

#define GetAutoRevision_Tag(str S) \
  Local[0] = GetAutoRevisionComponents(S, Local[1], Local[2], Local[3], Local[4]), \
  Local[1]

#define GetAutoRevision_Branch(str S) \
  Local[0] = GetAutoRevisionComponents(S, Local[1], Local[2], Local[3], Local[4]), \
  Local[2]

#define GetAutoRevision_Commit(str S) \
  Local[0] = GetAutoRevisionComponents(S, Local[1], Local[2], Local[3], Local[4]), \
  Local[3]

#define GetAutoRevision_Extra(str S) \
  Local[0] = GetAutoRevisionComponents(S, Local[1], Local[2], Local[3], Local[4]), \
  Local[4]

#define ShortCommitHashFromCommit(str Commit) \
  Copy(Commit, 1, 7)

#define TruncateStrWithEllipsis(str S, int MaxLength) \
  (Len(S) <= MaxLength) ? S : ( Copy(S, 1, MaxLength-1) + "…" )
