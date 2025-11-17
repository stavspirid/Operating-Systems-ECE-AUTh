mkdir episodes
cd episodes

for i in (seq 1 3)
  for j in (seq 1 4)
    touch "The_Office_Season $i Episode $j"
  end
end
