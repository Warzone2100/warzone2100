CREATE TABLE Components (id integer primary key autoincrement,  -- Unique ID of the component
                         type integer,                          -- 
                         name text,                             -- Component name
                         power_required integer,                -- Power required to build each component
                         build_points_required integer,         -- Number of build points required to manufacture each component
                         weight integer,                        -- The weight of the component
                         hit_points integer,                    -- The maximum amount of damage a component can take in one hit
                         system_points integer,                 -- The number taken up be the component
                         body_points integer,                   -- A measure of how much damage the component can take without being destroyed
                         WIP_name text,                         -- Work in progress name
                         technology_type_id integer,            -- The Tech Level of the component
                         gfx_file0 text,                        -- the name of the graphics file for player 0
                         gfx_file1 text,                        -- the name of the graphics file for player 1
                         gfx_file2 text,                        -- the name of the graphics file for player 2
                         gfx_file3 text,                        -- the name of the graphics file for player 3
                         gfx_file4 text,                        -- the name of the graphics file for player 4
                         gfx_file5 text,                        -- the name of the graphics file for player 5
                         gfx_file6 text,                        -- the name of the graphics file for player 6
                         gfx_file7 text                         -- the name of the graphics file for player 7
);
