-- Table structure for table `bodypropulsionimd`

CREATE TABLE `bodypropulsionimd` (
  body                  INTEGER NOT NULL, -- The body's ID number (i.e. refers to the `id` column in table `body`)
  propulsion            INTEGER NOT NULL, -- The propulsions's ID number (i.e. refers to the `id` column in table `propulsion`)
  leftIMD               TEXT,             -- Graphics for the propulsion object on the left of the body
  rightIMD              TEXT,             -- Graphics for the propulsion object on the right of the body
  UNIQUE(body, propulsion)
);
