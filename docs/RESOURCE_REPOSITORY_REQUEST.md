# External Resource Repository Requirement

Required repository:

`godrenkon/SuiraBox-OS-Resources`

Purpose:

Store Optional Resources outside the OS source repository and outside the base ISO.

Initial families:

- locale packs
- additional fonts
- themes
- icon packs
- wallpapers
- sound packs
- optional applications
- large help/tutorial/sample data

The OS repository must contain only the client contract, verification logic, fallback data, tests, and documentation needed to consume these resources.

The external repository must not become a dependency for boot or basic offline recovery.

Creation status:

- Repository does not currently exist in the accessible repository list.
- Current GitHub integration does not expose repository-creation mutation.
- The OS-side contract is therefore being implemented first.
- Once the repository exists, its stable manifest endpoint should be wired into the Resource client without changing Core/Resource boundaries.
