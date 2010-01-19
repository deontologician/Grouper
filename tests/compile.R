readandprint <- function(bits) {
  patt <- paste("alltest",bits,"bit_[1-5].csv", sep = "")
  files <- lapply(c(list.files(pattern = patt)), read.csv, head=T)
  total <- data.frame()
  for (file in files[]) {
    total <- merge(total, file, all=T)
  }
  total$number.of.rules <- factor(total$number.of.rules)
  total$number.of.tables <- factor(total$number.of.tables)

  #split the total into subsets based on the rules
  vect <- levels(total$number.of.rules)
  print(vect)
  x <- list()
  for (i in 1:length(vect)) {
    x[[i]] <- subset(total, total$number.of.rules == vect[[i]], all=TRUE)
  }
  #print them all to file!
  filename <- paste("alltest",bits,"bit_kbps_vs_rules.pdf", sep="")
  pdf(file = filename)
  for (p in x) {
    title = paste(bits, "bits,", p$number.of.rules[[1]], "Rules")
    xlabel = "Number of Tables"
    ylabel = "Kbps"
    plot(p$number.of.tables, p$kbps, main = title, xlab = xlabel, ylab = ylabel)
  }
  dev.off()
}

